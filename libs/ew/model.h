/*
 *	Author: Eric Winebrenner
 */

#pragma once

#include "mesh.h"
#include <string>
#include <vector>

namespace ew {
class Model {
public:
  Model(const std::string &filePath, bool instanced = false);
  void draw(int count = 1);

private:
  std::vector<ew::Mesh> m_meshes;
};
} // namespace ew
