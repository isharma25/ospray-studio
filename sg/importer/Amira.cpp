#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "Importer.h"
#include "sg/visitors/PrintNodes.h"
// ospcommon
#include "rkcommon/os/FileName.h"

namespace ospray {
namespace sg {

struct AmiraImporter : public Importer
{
  AmiraImporter() = default;
  ~AmiraImporter() override = default;

  void importScene() override;
};

OSP_REGISTER_SG_NODE_NAME(AmiraImporter, importer_amira);

struct AmiraData{
  float voxelType;
  vec3i dimensions;
  vec3f gridOrigin;
  vec3f gridSpacing;
  short *pData;
  OSPDataType dataType;
};

const char *findAndJump(const char *buffer, const char *searchString)
{
  const char *location = strstr(buffer, searchString);
  if (location)
    return location + strlen(searchString);
  return buffer;
}

int readAmira(const FileName &fileName, AmiraData &amiraData)
{
  FILE *file = fopen(fileName.c_str(), "rb");
  if (!file) {
    throw std::runtime_error(
        "AmiraVolume::could not open file " + fileName.str());
  }

  std::cout << "Reading file: " << fileName.c_str() << std::endl;

  // this buffer size works for a usual AmiraMesh file version 2.1
  // this buffer should encapsulate the following header properties:
  // 1. Version and Endian information
  // 2. define Lattice
  // 3. Parameters 
  // 4. Datatype 
  // 5. Data section beginning identifier(@1)
  char buffer[2048];
  fread(buffer, sizeof(char), 2047, file);
  buffer[2047] = '\0'; 

  if (!strstr(buffer, "# AmiraMesh BINARY-LITTLE-ENDIAN 2.1")) {
    std::cout << "Reader supports only an AmiraMesh file version 2.1\n";
    fclose(file);
    return 1;
  }

  // Find the Lattice definition, i.e., the dimensions of the uniform grid
  int xDim(0), yDim(0), zDim(0);
  sscanf(
      findAndJump(buffer, "define Lattice"), "%d %d %d", &xDim, &yDim, &zDim);
  std::cout << "\tGrid Dimensions:" <<  xDim << " " << yDim << " " << zDim << std::endl;
  amiraData.dimensions.x = xDim;
  amiraData.dimensions.y = yDim;
  amiraData.dimensions.z = zDim;

  if (xDim <= 0 || yDim <= 0 || zDim <= 0) {
    throw std::runtime_error("invalid volume dimensions");
  }

  // print the BoundingBox info
  float xmin(-1.0f), ymin(-1.0f), zmin(-1.0f);
  float xmax(1.0f), ymax(1.0f), zmax(1.0f);
  sscanf(findAndJump(buffer, "BoundingBox"),
      "%g %g %g %g %g %g",
      &xmin,
      &xmax,
      &ymin,
      &ymax,
      &zmin,
      &zmax);
  std::cout << "\tBoundingBox in x-Direction: " << xmin << " ... " << xmax << std::endl;
  std::cout << "\tBoundingBox in y-Direction: " << ymin << " ... " <<  ymax << std::endl;
  std::cout << "\tBoundingBox in z-Direction: " << zmin << " ... " <<  zmax << std::endl;
  // calculate grid spacing based on the bounding box
  amiraData.gridSpacing.x = (xmax-xmin)/(xDim-1);
  amiraData.gridSpacing.y = (ymax-ymin)/(yDim-1);
  amiraData.gridSpacing.z = (zmax-zmin)/(zDim-1);

  //set grid origin based on min values of the bounding box
  amiraData.gridOrigin.x = xmin;
  amiraData.gridOrigin.y = ymin;
  amiraData.gridOrigin.z = zmin;

  const bool bIsUniform = (strstr(buffer, "CoordType \"uniform\"") != NULL);
  std::cout << "\tGridType: " << (bIsUniform ? "uniform" : "UNKNOWN") << std::endl;

  std::string cellType{""};
  if (strstr(buffer, "short")) {
    cellType = "short";
    amiraData.dataType = OSP_SHORT;
  } else if (strstr(buffer, "float")) {
    cellType = "float";
    amiraData.dataType = OSP_FLOAT;
  } else if (strstr(buffer, "ushort")) {
    cellType = "ushort";
    amiraData.dataType = OSP_USHORT;
  } else if (strstr(buffer, "int")) {
    cellType = "int";
    amiraData.dataType = OSP_INT;
  } else if (strstr(buffer, "double")) {
    cellType = "double";
    amiraData.dataType = OSP_DOUBLE;
  } else if (strstr(buffer, "byte")) {
    cellType = "byte";
    amiraData.dataType = OSP_BYTE;
  } else if (strstr(buffer, "Labels")) {
    cellType = "Labels";
    amiraData.dataType = OSP_FLOAT;
  }

  int NumComponents(0);
  std::string latticeString = "Lattice { " + cellType + " Data }";
  std::string compString = "Lattice { " + cellType + "[";
  if (strstr(buffer, latticeString.c_str())) {
    NumComponents = 1;
  } else {
    sscanf(findAndJump(buffer, compString.c_str()), "%d", &NumComponents);
  }
  std::cout << "\tNumber of Components: "<<  NumComponents << std::endl;

  // Sanity check
  if (xDim <= 0 || yDim <= 0 || zDim <= 0 || xmin > xmax || ymin > ymax
      || zmin > zmax || !bIsUniform || NumComponents <= 0) {
    std::cout << "Provided dimensions/number-of-components are wrong or not a uniform grid\n";
    fclose(file);
    return 1;
  }

  // Find the beginning of the data section
  const long idxStartData = strstr(buffer, "# Data section follows") - buffer;
  if (idxStartData > 0) {
    // begin at "# Data section follows"
    fseek(file, idxStartData, SEEK_SET);
    // ignore "# Data section follows" part
    // fgets continues till end of line so max length works fine here
    fgets(buffer, 2047, file);
    // ignore "@1" part
    fgets(buffer, 2047, file);

    const size_t numValues = xDim * yDim * zDim * NumComponents;
    amiraData.pData = new short[numValues];
    if (amiraData.pData) {
      const size_t values =
          fread((void *)amiraData.pData, sizeof(short), numValues, file);
      if (numValues != values) {
        std::cout <<
            "Binary data could not be read.\n Corrupted data or end of file reached\n";
        delete[] amiraData.pData;
        fclose(file);
        return 1;
      }
    }
  }

  fclose(file);
  return 0;
}

// AmiraImporter definitions /////////////////////////////////////////////

void AmiraImporter::importScene()
{
  AmiraData amiraData;
  auto res = readAmira(fileName, amiraData);
  if (res < 0)
    std::cout << "Could not read Amira file" << std::endl;
  std::cout << "AmiraImporter::importScene()" << std::endl;

  // Create a root Transform/Instance off the Importer, then place the volume
  // under this.
  auto rootName = fileName.name() + "_rootXfm";
  auto nodeName = fileName.name() + "_volume";

  auto rootNode = createNode(rootName, "transform");

  auto volume = createNode(nodeName, "structuredRegular");
  volume->createChildData("data", amiraData.dimensions, 0, amiraData.pData);
  volume->createChild("voxelType", "OSPDataType", amiraData.dataType);
  volume->createChild("dimensions", "vec3i", amiraData.dimensions);
  volume->createChild("gridOrigin", "vec3f", amiraData.gridOrigin);
  volume->createChild("gridSpacing", "vec3f", amiraData.gridSpacing);
  
  // if importer has a shared transfer function; use that first
  // if (tfn)
  //   volume->add(tfn);
  // else {
  //   auto tf = createNode("transferFunction", "transfer_function_turbo");
  //   // adding value range here could be redundant as it is added also in
  //   // RenderScene
  //   auto valueRange = volume->child("value").valueAs<range1f>();
  //   tf->child("value") = valueRange;
  //   volume->add(tf);
  // }

  rootNode->add(volume);

  // Finally, add node hierarchy to importer parent
  add(rootNode);

  delete[] amiraData.pData;

  std::cout << "...finished import!\n";
}

} // namespace sg
} // namespace ospray
