#include "Myclasses.h"




Mesh::Mesh(vector<XMFLOAT3> pos,
	vector<XMFLOAT3> uvs,
	vector<XMFLOAT3> normals,
	vector<XMFLOAT3> trindices)
{
	for (unsigned int i = 0; i < trindices.size; i++)
	{
		VertexPositionUVNormal tempVert = new VertexPositionUVNormal();
		tempVert.pos = pos[trindices[i].x];
		tempVert.uv = uvs[trindices[i].y];
		tempVert.normal = normals[trindices[i].z];
		for (unsigned int i = 0; i < uniqueVertList.size; i++)
		{
			if (tempVert == uniqueVertList[i])
			{
				indexbuffer.push_back(i);
				break;
			}
			elseb
			{
				uniqueVertList.push_back(tempVert);
			indexbuffer.push_back(uniqueVertList.size - 1);
			}
		}
	}
}
Mesh::~Mesh()
{
	uniqueVertList.clear();
	indexbuffer.clear();
}
