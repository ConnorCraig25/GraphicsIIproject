#include "pch.h"
#include "Sample3DSceneRenderer.h"

#include "..\Common\DirectXHelper.h"

using namespace DX11UWA;

using namespace DirectX;
using namespace Windows::Foundation;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Sample3DSceneRenderer::Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_indexBunnyCount(0),
	m_tracking(false),
	m_deviceResources(deviceResources)
{
	memset(m_kbuttons, 0, sizeof(m_kbuttons));
	m_currMousePos = nullptr;
	m_prevMousePos = nullptr;
	memset(&m_camera, 0, sizeof(XMFLOAT4X4));

	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Sample3DSceneRenderer::CreateWindowSizeDependentResources(void)
{
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
	{
		fovAngleY *= 2.0f;
	}

	// Note that the OrientationTransform3D matrix is post-multiplied here
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.

	// This sample makes use of a right-handed coordinate system using row-major matrices.
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 0.01f, 100.0f);

	XMFLOAT4X4 orientation = m_deviceResources->GetOrientationTransform3D();

	XMMATRIX orientationMatrix = XMLoadFloat4x4(&orientation);

	XMStoreFloat4x4(&m_constBufferData.projection, XMMatrixTranspose(perspectiveMatrix * orientationMatrix));

	// Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
	static const XMVECTORF32 eye = { 0.0f, 0.7f, -1.5f, 0.0f };
	static const XMVECTORF32 at = { 0.0f, -0.1f, 0.0f, 0.0f };
	static const XMVECTORF32 up = { 0.0f, 1.0f, 0.0f, 0.0f };

	XMStoreFloat4x4(&m_camera, XMMatrixInverse(nullptr, XMMatrixLookAtLH(eye, at, up)));
	XMStoreFloat4x4(&m_constBufferData.view, XMMatrixTranspose(XMMatrixLookAtLH(eye, at, up)));
}

// Called once per frame, rotates the cube and calculates the model and view matrices.
void Sample3DSceneRenderer::Update(DX::StepTimer const& timer)
{
	if (!m_tracking)
	{
		// Convert degrees to radians, then convert seconds to rotation angle
		float radiansPerSecond = XMConvertToRadians(m_degreesPerSecond);
		double totalRotation = timer.GetTotalSeconds() * radiansPerSecond;
		float radians = static_cast<float>(fmod(totalRotation, XM_2PI));

		Rotate(135.0*3.14/180);
	}

	m_constBufferSkyboxData.view = m_constBufferData.view;
	m_constBufferSkyboxData.projection = m_constBufferData.projection;
	XMStoreFloat4x4(&m_constBufferSkyboxData.model, XMMatrixTranspose(XMMatrixTranslation(m_camera._41, m_camera._42, m_camera._43)));

	XMStoreFloat4(&m_LightProperties.EyePosition, XMVectorSet(m_camera._41, m_camera._42, m_camera._43, 1.0f));

	for (int i = 0; i < numLights; ++i)
	{
		Light light;
		XMFLOAT4 LightPosition;
		memset(&light, 0, sizeof(Light));
		light.LightTypeEnabled.y = LightEnabled[i];
		light.LightTypeEnabled.x = i;
		light.Color = XMFLOAT4(LightColors[i]);
		light.AttenuationData.x = XMConvertToRadians(45.0f);
		light.AttenuationData.y = 1.0f;
		light.AttenuationData.z = 0.08f;
		light.AttenuationData.w = 0.0f;

		//Directional light location
		if (i == 0)
		{
			if (dirLightSwitch)
			{
				directionalLightPos.x += 1;
				if (directionalLightPos.x >= 20)
				{
					dirLightSwitch = false;
				}
			}
			else
			{
				directionalLightPos.x -= 1;
				if (directionalLightPos.x <= -20)
				{
					dirLightSwitch = true;
				}
			}

			LightPosition = directionalLightPos;
		}

		//point light location
		if (i == 1)
		{
			if (pointLightSwitch)
			{
				pointLightPos.x += .25f;
				if (pointLightPos.x >= 20)
				{
					pointLightSwitch = false;
				}
			}
			else
			{
				pointLightPos.x -= .25f;
				if (pointLightPos.x <= -20)
				{
					pointLightSwitch = true;
				}
			}
			LightPosition = pointLightPos;
		}

		//Spot Light Information
		if (i == 2) // Spot light Location
		{
			LightPosition = spotPos;
		}

		light.radius.x = spotRad;
		light.ConeRatio.x = innerConeRat;
		light.ConeRatio.y = outterConeRat;
		light.coneAngle = coneAng;

		light.Position = LightPosition;
		XMVECTOR LightDirection = XMVectorSet(-LightPosition.x, -LightPosition.y, -LightPosition.z, 0.0f);
		LightDirection = XMVector3Normalize(LightDirection);
		XMStoreFloat4(&light.Direction, LightDirection);

		m_LightProperties.Lights[i] = light;
	}

	//m_d3dDeviceContext->UpdateSubresource(m_d3dLightPropertiesConstantBuffer.Get(), 0, nullptr, &m_LightProperties, 0, 0);
	m_deviceResources->GetD3DDeviceContext()->UpdateSubresource(lightbuffer.Get(), 0, NULL, &m_LightProperties, 0, 0);


	// Update or move camera here
	UpdateCamera(timer, 5.0f, 0.75f);

}

// Rotate the 3D cube model a set amount of radians.
void Sample3DSceneRenderer::Rotate(float radians)
{
	// Prepare to pass the updated model matrix to the shader
	XMStoreFloat4x4(&m_constBufferData.model, XMMatrixTranspose(XMMatrixRotationY(radians)));
}

void Sample3DSceneRenderer::UpdateCamera(DX::StepTimer const& timer, float const moveSpd, float const rotSpd)
{
	const float delta_time = (float)timer.GetElapsedSeconds();

	if (m_kbuttons['W'])
	{
		XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, moveSpd * delta_time);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['S'])
	{
		XMMATRIX translation = XMMatrixTranslation(0.0f, 0.0f, -moveSpd * delta_time);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['A'])
	{
		XMMATRIX translation = XMMatrixTranslation(-moveSpd * delta_time, 0.0f, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['D'])
	{
		XMMATRIX translation = XMMatrixTranslation(moveSpd * delta_time, 0.0f, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons['X'])
	{
		XMMATRIX translation = XMMatrixTranslation( 0.0f, -moveSpd * delta_time, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}
	if (m_kbuttons[VK_SPACE])
	{
		XMMATRIX translation = XMMatrixTranslation( 0.0f, moveSpd * delta_time, 0.0f);
		XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
		XMMATRIX result = XMMatrixMultiply(translation, temp_camera);
		XMStoreFloat4x4(&m_camera, result);
	}

	if (m_currMousePos) 
	{
		if (m_currMousePos->Properties->IsRightButtonPressed && m_prevMousePos)
		{
			float dx = m_currMousePos->Position.X - m_prevMousePos->Position.X;
			float dy = m_currMousePos->Position.Y - m_prevMousePos->Position.Y;

			XMFLOAT4 pos = XMFLOAT4(m_camera._41, m_camera._42, m_camera._43, m_camera._44);

			m_camera._41 = 0;
			m_camera._42 = 0;
			m_camera._43 = 0;

			XMMATRIX rotX = XMMatrixRotationX(dy * rotSpd * delta_time);
			XMMATRIX rotY = XMMatrixRotationY(dx * rotSpd * delta_time);

			XMMATRIX temp_camera = XMLoadFloat4x4(&m_camera);
			temp_camera = XMMatrixMultiply(rotX, temp_camera);
			temp_camera = XMMatrixMultiply(temp_camera, rotY);

			XMStoreFloat4x4(&m_camera, temp_camera);

			m_camera._41 = pos.x;
			m_camera._42 = pos.y;
			m_camera._43 = pos.z;
		}
		m_prevMousePos = m_currMousePos;
	}


}

void Sample3DSceneRenderer::SetKeyboardButtons(const char* list)
{
	memcpy_s(m_kbuttons, sizeof(m_kbuttons), list, sizeof(m_kbuttons));
}

void Sample3DSceneRenderer::SetMousePosition(const Windows::UI::Input::PointerPoint^ pos)
{
	m_currMousePos = const_cast<Windows::UI::Input::PointerPoint^>(pos);
}

void Sample3DSceneRenderer::SetInputDeviceData(const char* kb, const Windows::UI::Input::PointerPoint^ pos)
{
	SetKeyboardButtons(kb);
	SetMousePosition(pos);
}

void DX11UWA::Sample3DSceneRenderer::StartTracking(void)
{
	m_tracking = true;
}

// When tracking, the 3D cube can be rotated around its Y axis by tracking pointer position relative to the output screen width.
void Sample3DSceneRenderer::TrackingUpdate(float positionX)
{
	if (m_tracking)
	{
		float radians = XM_2PI * 2.0f * positionX / m_deviceResources->GetOutputSize().Width;
		Rotate(radians);
	}
}

void Sample3DSceneRenderer::StopTracking(void)
{
	m_tracking = false;
}

// Renders one frame using the vertex and pixel shaders.
void Sample3DSceneRenderer::Render(void)
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	XMStoreFloat4x4(&m_constBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));


	
	// Each vertex is one instance of the VertexPositionColor struct.
	UINT stride = sizeof(VertexPositionUVNormal);
	UINT offset = 0;

	
	context->UpdateSubresource1(m_ConstantBuffer.Get(), 0, NULL, &m_constBufferSkyboxData, 0, 0, 0);
	context->IASetVertexBuffers(0, 1, m_VertSkyboxBuffer.GetAddressOf(), &stride, &offset);
	// Each index is one 16-bit unsigned integer (short).
	context->IASetIndexBuffer(m_IndexSkyboxBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(0, 1, m_ConstantBuffer.GetAddressOf(), nullptr, nullptr);
	// Attach our pixel shader.
	context->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_skyboxTex.GetAddressOf());
	// Draw the objects.
	context->DrawIndexed(m_indexSkyboxCount, 0, 0);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


// Prepare the constant buffer to send it to the graphics device.
	context->UpdateSubresource1(m_ConstantBuffer.Get(), 0, NULL, &m_constBufferData, 0, 0, 0);
	context->IASetVertexBuffers(0, 1, m_VertBunnyBuffer.GetAddressOf(), &stride, &offset);
	// Each index is one 16-bit unsigned integer (short).
	context->IASetIndexBuffer(m_IndexBunnyBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_inputLayout.Get());
	// Attach our vertex shader.
	context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	// Send the constant buffer to the graphics device.
	context->VSSetConstantBuffers1(0, 1, m_ConstantBuffer.GetAddressOf(), nullptr, nullptr);
	// Attach our pixel shader.
	context->PSSetShader(m_light_pixelShader.Get(), nullptr, 0);
	context->PSSetShaderResources(0, 1, m_bunnyTex.GetAddressOf());
	// Draw the objects.
	context->DrawIndexed(m_indexBunnyCount, 0, 0);

	for (unsigned int i = 0; i < 4; i++)
	{
		XMStoreFloat4x4(&m_constBufferData.view, XMMatrixTranspose(XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_camera))));


		// Prepare the constant buffer to send it to the graphics device.
		context->UpdateSubresource1(m_ConstantBuffer.Get(), 0, NULL, &m_constBufferData, 0, 0, 0);
		// Each vertex is one instance of the VertexPositionColor struct.
		context->IASetVertexBuffers(0, 1, m_VertCastleBuffer[i].GetAddressOf(), &stride, &offset);
		// Each index is one 16-bit unsigned integer (short).
		context->IASetIndexBuffer(m_IndexCastleBuffer[i].Get(), DXGI_FORMAT_R32_UINT, 0);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(m_inputLayout.Get());
		// Attach our vertex shader.
		context->VSSetShader(m_vertexShader.Get(), nullptr, 0);
		// Send the constant buffer to the graphics device.
		context->VSSetConstantBuffers1(0, 1, m_ConstantBuffer.GetAddressOf(), nullptr, nullptr);
		// Attach our pixel shader.
		context->PSSetShader(m_light_pixelShader.Get(), nullptr, 0);
		context->PSSetShaderResources(0, 1, m_castleTex[i].GetAddressOf());
		//lighting 
		context->PSSetConstantBuffers(0, 1, lightbuffer.GetAddressOf());
		// Draw the objects.
		context->DrawIndexed(m_indexCastleCount[i], 0, 0);
	}


	
}

void Sample3DSceneRenderer::CreateDeviceDependentResources(void)
{
	// Load shaders asynchronously.
	auto loadVSTask = DX::ReadDataAsync(L"SampleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync(L"SamplePixelShader.cso");
	auto loadLightPSTask = DX::ReadDataAsync(L"LightPixelShader.cso");

	// After the vertex shader file is loaded, create the shader and input layout.
	auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateVertexShader(&fileData[0], fileData.size(), nullptr, &m_vertexShader));

		static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "UV", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateInputLayout(vertexDesc, ARRAYSIZE(vertexDesc), &fileData[0], fileData.size(), &m_inputLayout));
	});
	// After the pixel shader file is loaded, create the shader and constant buffer.
	auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_pixelShader));

		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(ModelViewProjectionConstantBuffer), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &m_ConstantBuffer));
	});

	auto createlightPSTask = loadLightPSTask.then([this](const std::vector<byte>& fileData)
	{
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreatePixelShader(&fileData[0], fileData.size(), nullptr, &m_light_pixelShader));
		CD3D11_BUFFER_DESC constantBufferDesc(sizeof(LightProperties), D3D11_BIND_CONSTANT_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&constantBufferDesc, nullptr, &lightbuffer));
	});

	// Once both shaders are loaded, create the mesh.
	auto createBunnyTask = (createPSTask && createVSTask  && createlightPSTask).then([this]()
	{

		Mesh sphere = Mesh("Assets/bunny.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertBunnyBuffer));
	
		m_indexBunnyCount = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexBunnyBuffer));

		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), 
								 L"Assets/bunny.dds",
								 nullptr,
								 m_bunnyTex.GetAddressOf());
	});

	auto createCastle1Task = (createPSTask && createVSTask && createlightPSTask).then([this]()
	{

		Mesh sphere = Mesh("Assets/Castle1.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertCastleBuffer[0]));

		m_indexCastleCount[0] = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexCastleBuffer[0]));

		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(),
			L"Assets/Castle1.dds",
			nullptr,
			m_castleTex[0].GetAddressOf());
	});

	auto createCastle2Task = (createPSTask && createVSTask && createlightPSTask).then([this]()
	{

		Mesh sphere = Mesh("Assets/Castle2.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertCastleBuffer[1]));

		m_indexCastleCount[1] = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexCastleBuffer[1]));

		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(),
			L"Assets/Castle2.dds",
			nullptr,
			m_castleTex[1].GetAddressOf());
	});

	auto createCastle3Task = (createPSTask && createVSTask && createlightPSTask).then([this]()
	{

		Mesh sphere = Mesh("Assets/Castle3.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertCastleBuffer[2]));

		m_indexCastleCount[2] = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexCastleBuffer[2]));

		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(),
			L"Assets/Castle3.dds",
			nullptr,
			m_castleTex[2].GetAddressOf());
	});

	auto createCastle4Task = (createPSTask && createVSTask && createlightPSTask).then([this]()
	{

		Mesh sphere = Mesh("Assets/Castle4.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
		vertexBufferData.pSysMem = sphere.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*sphere.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertCastleBuffer[3]));

		m_indexCastleCount[3] = sphere.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = sphere.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*sphere.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexCastleBuffer[3]));

		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(),
			L"Assets/Castle4.dds",
			nullptr,
			m_castleTex[3].GetAddressOf());
	});

	auto createSkyBoxTask = (createPSTask && createVSTask && createlightPSTask).then([this]()
	{
		Mesh skybox = Mesh("Assets/Skybox.obj");

		D3D11_SUBRESOURCE_DATA vertexBufferData;
		vertexBufferData.pSysMem = skybox.uniqueVertList.data();
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(VertexPositionUVNormal)*skybox.uniqueVertList.size(), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, &vertexBufferData, &m_VertSkyboxBuffer));

		m_indexSkyboxCount = skybox.indexbuffer.size();

		D3D11_SUBRESOURCE_DATA indexBufferData = { 0 };
		indexBufferData.pSysMem = skybox.indexbuffer.data();
		indexBufferData.SysMemPitch = 0;
		indexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC indexBufferDesc(sizeof(unsigned int)*skybox.indexbuffer.size(), D3D11_BIND_INDEX_BUFFER);
		DX::ThrowIfFailed(m_deviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexBufferData, &m_IndexSkyboxBuffer));
		
		CreateDDSTextureFromFile(m_deviceResources->GetD3DDevice(), 
								 L"Assets/skyboxtexture.dds",
								 nullptr,
								 m_skyboxTex.GetAddressOf());
	});


	// Once the geometry is loaded, the object is ready to be rendered.



	(createBunnyTask &&
	 createCastle1Task && 
	 createCastle2Task && 
	 createCastle3Task && 
	 createCastle4Task && 
	 createSkyBoxTask).then([this]()
	{
		m_loadingComplete = true;
	});
}

void Sample3DSceneRenderer::ReleaseDeviceDependentResources(void)
{
	m_loadingComplete = false;
	m_vertexShader.Reset();
	m_inputLayout.Reset();
	m_pixelShader.Reset();
	m_ConstantBuffer.Reset();
	m_VertBunnyBuffer.Reset();
	m_IndexBunnyBuffer.Reset();
}