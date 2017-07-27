#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"


namespace DX11UWA
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		void CreateDeviceDependentResources(void);
		void CreateWindowSizeDependentResources(void);
		void ReleaseDeviceDependentResources(void);
		void Update(DX::StepTimer const& timer);
		void Render(void);
		void StartTracking(void);
		void TrackingUpdate(float positionX);
		void StopTracking(void);
		inline bool IsTracking(void) { return m_tracking; }

		// Helper functions for keyboard and mouse input
		void SetKeyboardButtons(const char* list);
		void SetMousePosition(const Windows::UI::Input::PointerPoint^ pos);
		void SetInputDeviceData(const char* kb, const Windows::UI::Input::PointerPoint^ pos);


	private:
		void Rotate(float radians);
		void UpdateCamera(DX::StepTimer const& timer, float const moveSpd, float const rotSpd);

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;
		
		//some shared Direct3D resources
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>   m_light_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pyramid_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_ConstantBuffer;	

		ModelViewProjectionConstantBuffer	m_constBufferData;


		
		//lighting
		struct Light
		{
			XMFLOAT4    Position;    //16
			XMFLOAT4    Direction;   //16
			XMFLOAT4    radius;
			XMFLOAT4    Color; //16

			XMFLOAT4    AttenuationData;
			// x =  SpotAngle;
			// y =  ConstantAttenuation;
			// z =  LinearAttenuation;
			// w =  QuadraticAttenuation; 

			XMFLOAT4 LightTypeEnabled;
			// x  = type
			// y = Enabled;

			XMFLOAT4   ConeRatio; // x = inner ratio,  y = outerratio

			XMFLOAT4    coneAngle;

		};

		struct LightProperties
		{
			XMFLOAT4 EyePosition;
			XMFLOAT4 GlobalAmbient;
			Light  Lights[3];
		};
		LightProperties m_LightProperties;

		XMVECTORF32 LightColors[3] =
		{
			// Directional light;      Point Light;    Spot Light;
			Colors::White, Colors::Blue, Colors::Red

		};

		bool LightEnabled[3] =
		{
			true, true, true
		};

		//Dynamic Variables
		bool dirLightSwitch = false; // false = negative direction true = positive direction
		bool pointLightSwitch = false; // false = negative direction true = positive direction
		bool spotLightSwitch = false;
		float spotRad = 10.0f;
		float innerConeRat = .8f;
		float outterConeRat = .45f;
		XMFLOAT4 coneAng = { 0,-1.0f, -1.0f, 0 };

		XMFLOAT4 pointLightPos = { 5.0f,5.0f, 5.0f,1 };
		XMFLOAT4 directionalLightPos = { -7.0f, 20.0f, 0.0f,1 };
		XMFLOAT4 spotPos = { 0.0, 5.0f, 0.0f,1 };

		int numLights = 3;
		float radius = 1.0f;
		float offset = 2.0f * XM_PI / numLights;


		Microsoft::WRL::ComPtr<ID3D11Buffer> lightbuffer;
		// Direct3D resources for pyramid
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_VertPyramidBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_IndexPyramidBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constPyramidBuffer;
		// System resources for pyramid geometry.
		uint32	m_indexPyramidCount;
		uint32 m_numPyramids;
		ModelViewProjectionConstantBuffer	m_constBufferPyramidData[3];

		// Direct3D resources for BUNNY geometry.//
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_VertBunnyBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_IndexBunnyBuffer;		
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_bunnyTex;
		// System resources for bunny geometry.
		uint32	m_indexBunnyCount;
		ModelViewProjectionConstantBuffer	m_constBufferBunnyData;

		// Direct3D resources for CASTLE geometry.//
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_VertCastleBuffer[4];
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_IndexCastleBuffer[4];
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_castleTex[4];
		// System resources for castle geometry.
		uint32	m_indexCastleCount[4];
		ModelViewProjectionConstantBuffer	m_constBufferCastleData;

		//Direct3D resources for Sky Box.//
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_VertSkyboxBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_IndexSkyboxBuffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_skyboxTex;
		ModelViewProjectionConstantBuffer	m_constBufferSkyboxData;
		// System resources for Skybox geometry.
		uint32 m_indexSkyboxCount;

		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		float	m_degreesPerSecond;
		bool	m_tracking;

		// Data members for keyboard and mouse input
		char	m_kbuttons[256];
		Windows::UI::Input::PointerPoint^ m_currMousePos;
		Windows::UI::Input::PointerPoint^ m_prevMousePos;

		// Matrix data member for the camera
		DirectX::XMFLOAT4X4 m_camera;
	};
}

