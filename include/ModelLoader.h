#pragma once
#include <iostream>
#include <Engine/Vertex.h>
#include <Engine/Vector3.h>
#include <vector>
#include <string>
#include <fstream>

class ModelLoader{
public:

    std::vector<std::string> ParseCharacter(std::string input, char parsingChar){
        std::vector<std::string> output;

        int currentPos = 0; int prevPos = 0;
        while ((currentPos = input.find(parsingChar, currentPos)) != std::string::npos) {
            output.push_back(input.substr(prevPos, currentPos-prevPos));
            currentPos++; prevPos = currentPos;
        } output.push_back(input.substr(prevPos));

        return output;
    }

    std::vector<Vertex> LoadObj(const std::string& directory){
        std::cout << "Loading: " << directory << "\n";
        std::vector<Vertex> vertices;

        std::ifstream file(directory);
        std::string line;

        if (!file.is_open()) {
            std::cout << "Failed to open " << directory << "\n";
            return {};
        }

        std::vector<Vector3> vertPositions;
        // std::vector<Normal> normalData;
        // std::vector<UV> UVData;

        while (std::getline(file, line)) {
            if (line.size() < 2) continue;
            std::string lineType = line.substr(0,2);

            // Get Vertex position Loop
            if(lineType == "v ") { // Vertice Positions
                Vector3 pos;

                float* coords[3] = { &pos.x, &pos.y, &pos.z };
                std::vector<std::string> positions = ParseCharacter(line.substr(2), ' ');
                for(int i = 0; i<positions.size(); i++) {
                    *coords[i] = std::stof(positions[i]);
                } vertPositions.push_back(pos);
            }else if (lineType == "vn") { // Normals
                // Normal n;

                // std::istringstream ss(line.substr(3));
                // ss >> n.x >> n.y >> n.z;
                // normalData.push_back(n);
            } else if (lineType == "vt"){ // Get UV Data
                // UV data;

                // float* UVInput[2] = {&data.u, &data.v};
                // std::vector<std::string> UVFileData = ParseCharacter(line.substr(3), ' ');
                // for(int i = 0; i<UVFileData.size(); i++){
                //     *UVInput[i] = std::stof(UVFileData[i]);
                // } data.v = 1.0f - data.v;
                // UVData.push_back(data);
            } else if(lineType == "f "){ // Assemble Faces
                std::vector<std::string> vertGroups = ParseCharacter(line.substr(2), ' ');
                if (vertGroups.size() < 3) continue;

                for (size_t i = 1; i + 1 < vertGroups.size(); ++i) {
                    std::string triIndices[3] = { vertGroups[0], vertGroups[i], vertGroups[i + 1] };

                    for (int t = 0; t < 3; t++) {
                        std::vector<std::string> indexData = ParseCharacter(triIndices[t], '/');

                        int vIndex  = std::stoi(indexData[0]) - 1;
                        int vtIndex = (indexData.size() > 1 && !indexData[1].empty()) ? std::stoi(indexData[1]) - 1 : -1;
                        int vnIndex = (indexData.size() > 2 && !indexData[2].empty()) ? std::stoi(indexData[2]) - 1 : -1;

                        Vertex vert{};
                        vert.x = vertPositions[vIndex].x;
                        vert.y = vertPositions[vIndex].y;
                        vert.z = vertPositions[vIndex].z;
                        vert.r = vert.g = vert.b = float(vIndex)/vertPositions.size();

                        // if (vtIndex >= 0 && vtIndex < (int)UVData.size()) {
                        //     vert.u = UVData[vtIndex].u;
                        //     vert.v = UVData[vtIndex].v;
                        // } else { vert.u = vert.v = 0.0f; }

                        // if (vnIndex >= 0 && vnIndex < (int)normalData.size()) {
                        //     vert.nx = normalData[vnIndex].x;
                        //     vert.ny = normalData[vnIndex].y;
                        //     vert.nz = normalData[vnIndex].z;
                        // } else { vert.nx = vert.ny = vert.nz = 0.0f; }

                        vertices.push_back(vert);
                    }
                }
            }
        }

        std::cout << "Loaded " << directory << "\n";
        return vertices;
    }
};