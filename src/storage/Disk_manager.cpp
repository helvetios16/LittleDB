#include "storage/Disk_manager.h"

#include <algorithm>
#include <iostream>
#include <filesystem>
#include <regex>

namespace fs = std::filesystem;
using namespace std;

Disk_manager::Disk_manager() : rootNode(disk.getRoot()) {
}

void Disk_manager::selectDiskStructure(bool defaultDisk) {
  if (defaultDisk) {
    disk.generateDiskStructure();
    disk.capacityDisk();
  } else {
    int plates, tracks, sector, bytes, bytesPerBlock;
    cout << "Ingrese el número de platos: ";
    cin >> plates;
    cout << "Ingrese el número de pistas por superficie: ";
    cin >> tracks;
    cout << "Ingrese el número de sectores por pista: ";
    cin >> sector;
    cout << "Ingrese el número de bytes por sector: ";
    cin >> bytes;
    cout << "Ingrese el numero de bytes por bloque: ";
    cin >> bytesPerBlock;
    disk = Disk(plates, tracks, sector, bytes, bytesPerBlock);
    disk.generateDiskStructure();
    disk.capacityDisk();
  }
}

void Disk_manager::getCapacityDisk() {
  disk.capacityDisk();
}

string eliminarSubstring(string& str, const string& substr) {
  regex pattern(substr + "\\d*\\.txt");
  str = regex_replace(str, pattern, "");
  return str;
}

void Disk_manager::insertRecord(string &relation, string &record, int recordSize) {
  string heapFilePath = "../../data/heapfiles/" + relation + ".txt";
  ifstream heapFile(heapFilePath);

  string blockPath;
  if (heapFile.is_open()) {
    getline(heapFile, blockPath);
    heapFile.close();
  } else {
    blockPath = findFreeBlock();
  }

  if (blockPath.empty()) {
    cerr << "Error: No se encontró un bloque libre." << endl;
    return;
  }

  string sectorPath = redirectSectorWithSpace(blockPath, recordSize);
  if (sectorPath.empty()) {
    cerr << "Error: No se encontró un sector con espacio suficiente." << endl;
    return;
  }

  sectorPath = eliminarSubstring(sectorPath, "/block");
  fstream sectorFile(sectorPath, ios::app);
  if (!sectorFile.is_open()) {
    cerr << "Error: No se pudo abrir el archivo " << sectorPath << "." << endl;
    return;
  }

  sectorFile.seekp(0, ios::end);
  sectorFile << record << endl;
  sectorFile.close();

  HeapFile hf(relation);
  hf.addBlock(blockPath);
  hf.saveToFile();

  cout << "Datos insertados en " + sectorPath + " exitosamente Bv." << endl;

}

string Disk_manager::findFreeBlock() {
  return searchFreeBlockInTree(rootNode);
}

string Disk_manager::searchFreeBlockInTree(const TreeNode &node) {
  if (isBlockFree(node.directory)) {
    return node.directory.string();
  }

  for (const auto &child: node.children) {
    string freeBlock = searchFreeBlockInTree(child);
    if (!freeBlock.empty()) {
      return freeBlock;
    }
  }

  return "";
}

bool Disk_manager::isBlockFree(const fs::path &blockPath) {
  ifstream blockFile(blockPath);

  if (blockFile.is_open()) {
    string firstLine;
    getline(blockFile, firstLine);

    blockFile.close();

    return (firstLine.find("FREE") != string::npos);
  }

  return false;
}


string Disk_manager::openBlock(const string &blockPath, const string &relationName) {
  fstream blockFile(blockPath, ios::in | ios::out);

  if (!blockFile.is_open()) {
    cerr << "Error: No se pudo abrir el archivo " << blockPath << "." << endl;
    return "";
  }

  string firstLine;
  getline(blockFile, firstLine);

  if (firstLine == "FREE") {
    blockFile.seekp(0);
    blockFile << relationName;
  }

  blockFile.close();

  return blockPath;
}

bool Disk_manager::checkSpaceInBlock(const string &blockPath, int recordSize) {
  ifstream blockFile(blockPath);

  if (!blockFile.is_open()) {
    cerr << "Error: No se pudo abrir el archivo " << blockPath << "." << endl;
    return false;
  }

  string line;
  while (getline(blockFile, line)) {
    if (line.substr(0, 13) == "blockCapacity") {
      int blockCapacity = stoi(line.substr(14));
      blockFile.close();
      return blockCapacity > recordSize;
    }
  }

  blockFile.close();
  return false;
}

string Disk_manager::redirectSectorWithSpace(const string &blockPath, int recordSize) {
  ifstream blockFile(blockPath);

  if (!blockFile.is_open()) {
    cerr << "Error: No se pudo abrir el archivo " << blockPath << "." << endl;
    return "";
  }

  string line;
  bool encontrado = false;

  while (getline(blockFile, line) && !encontrado) {
    if (line.substr(0, 1) == "C") {
      line = line.substr(1);
      int sectorCapacity = stoi(line.substr(line.find("#") + 1));
      if (sectorCapacity > recordSize) {
        blockFile.close();
        encontrado = true;
        return blockPath + "/" + line.substr(0, line.find("#"));
      }
    }
  }

  blockFile.close();
  return "";
}