#include "pch.h"

Mesh::Mesh(vector<XMFLOAT3> pos,
	vector<XMFLOAT3> uvs,
	vector<XMFLOAT3> normals,
	vector<XMINT3> trindices)
{
	for (unsigned int i = 0; i < trindices.size(); i++)
	{
		VertexPositionUVNormal tempVert =  VertexPositionUVNormal();

		tempVert.pos = pos[trindices[i].x- 1 ];
		tempVert.uv = uvs[trindices[i].y-1];
		tempVert.normal = normals[trindices[i].z-1];

		uniqueVertList.push_back(tempVert);
		indexbuffer.push_back(i);
		//if (uniqueVertList.size() == 0)
		//{
		//	uniqueVertList.push_back(tempVert);
		//	indexbuffer.push_back(uniqueVertList.size() - 1);
		//}
		//else
		//for (unsigned int i = 0; i < uniqueVertList.size(); i++)
		//{
		//	 if (tempVert.normal.x == uniqueVertList[i].normal.x
		//			&& tempVert.normal.y == uniqueVertList[i].normal.y
		//			&& tempVert.normal.z == uniqueVertList[i].normal.z
		//			&& tempVert.pos.x == uniqueVertList[i].pos.x
		//			&& tempVert.pos.y == uniqueVertList[i].pos.y
		//			&& tempVert.pos.z == uniqueVertList[i].pos.z
		//			&& tempVert.uv.x == uniqueVertList[i].uv.x
		//			&& tempVert.uv.y == uniqueVertList[i].uv.y
		//			&& tempVert.uv.z == uniqueVertList[i].uv.z)
		//	{
		//		indexbuffer.push_back(i);
		//		break;
		//	}
		//	else
		//	{
		//		uniqueVertList.push_back(tempVert);
		//		indexbuffer.push_back(uniqueVertList.size() - 1);
		//	}
		//}
	}
}
Mesh::~Mesh()
{
	uniqueVertList.clear();
	indexbuffer.clear();
}