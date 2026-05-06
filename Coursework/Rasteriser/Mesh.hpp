#pragma once
#include <Eigen/Dense>
#include <vector>
#include <fstream>

struct Mesh {
	std::vector<Eigen::Vector3f> verts;
	std::vector<Eigen::Vector3f> norms;
	std::vector<Eigen::Vector2f> texs;
	std::vector<std::array<unsigned int, 3>> vFaces;
	std::vector<std::array<unsigned int, 3>> nFaces;
	std::vector<std::array<unsigned int, 3>> tFaces;
};


Mesh loadMeshFile(const std::string& filename)
{
	Mesh mesh;

	std::ifstream file(filename);

	if (file.fail()) throw std::runtime_error("Unable to find mesh file: " + filename);

	std::string line;
	while (!file.eof())
	{
		std::getline(file, line);
		std::stringstream lineSS(line.c_str());
		char lineStart;
		lineSS >> lineStart;
		char ignoreChar;
		if (lineStart == 'v') {
			if (line[1] == ' ') {
				Eigen::Vector3f v;
				for (int i = 0; i < 3; ++i) lineSS >> v[i];
				mesh.verts.push_back(v);
			}
			else if (line[1] == 'n') {
				lineSS >> ignoreChar;
				Eigen::Vector3f n;
				for (int i = 0; i < 3; ++i) lineSS >> n[i];
				mesh.norms.push_back(n);
			}
			else if (line[1] == 't') {
				lineSS >> ignoreChar;
				Eigen::Vector2f t;
				for (int i = 0; i < 2; ++i) lineSS >> t[i];
				mesh.texs.push_back(t);
			}

		}

		if (lineStart == 'f') {
			std::array<unsigned int, 3> vFace, nFace, tFace;
			unsigned int idx, idxTex, idxNorm;
			int i = 0;
			while (lineSS >> idx >> ignoreChar >> idxTex >> ignoreChar >> idxNorm) {
				vFace[i] = idx - 1;
				nFace[i] = idxNorm - 1;
				tFace[i] = idxTex - 1;
				++i;
			}
			if (i > 0) {
				mesh.vFaces.push_back(vFace);
				mesh.nFaces.push_back(nFace);
				mesh.tFaces.push_back(tFace);
			}
		}
	}

	return mesh;
}

