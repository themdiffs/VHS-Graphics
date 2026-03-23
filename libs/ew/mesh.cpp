/*
 *	Author: Eric Winebrenner
 */

#include "mesh.h"

// batteries
#include "batteries/opengl.h"

namespace ew {
Mesh::Mesh(const MeshData &meshData, bool instanced) {
  load(meshData, instanced);
}
void Mesh::load(const MeshData &meshData, bool instanced) {
  if (!m_initialized) {
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glGenBuffers(1, &m_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // texcoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)(offsetof(Vertex, uv)));
    glEnableVertexAttribArray(2);

    if (instanced) {
      // mat4 -> 4 * glm::vec4
      //
      glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                            (const void *)(0 * sizeof(glm::vec4)));
      glEnableVertexAttribArray(3);

      glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                            (const void *)(1 * sizeof(glm::vec4)));
      glEnableVertexAttribArray(4);

      glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                            (const void *)(2 * sizeof(glm::vec4)));
      glEnableVertexAttribArray(5);

      glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                            (const void *)(3 * sizeof(glm::vec4)));
      glEnableVertexAttribArray(6);

      // tells the gpu how the data will be handled
      // tells them they're all one, they share the same space
      glVertexAttribDivisor(3, 1);
      glVertexAttribDivisor(4, 1);
      glVertexAttribDivisor(5, 1);
      glVertexAttribDivisor(6, 1);
    }

    // tangent attribute
    glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (const void *)offsetof(Vertex, tangent));
    glEnableVertexAttribArray(7);
    m_initialized = true;
  }

  glBindVertexArray(m_vao);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

  if (meshData.vertices.size() > 0) {
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * meshData.vertices.size(),
                 meshData.vertices.data(), GL_STATIC_DRAW);
  }
  if (meshData.indices.size() > 0) {
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 sizeof(unsigned int) * meshData.indices.size(),
                 meshData.indices.data(), GL_STATIC_DRAW);
  }
  m_numVertices = meshData.vertices.size();
  m_numIndices = meshData.indices.size();

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
void Mesh::draw(ew::DrawMode drawMode, int count) const {
  glBindVertexArray(m_vao);
  switch (drawMode) {
  case DrawMode::TRIANGLES: {
    if (count > 1) {
      glDrawElementsInstanced(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, NULL,
                              count);
    } else {
      glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, NULL);
    }
  } break;
  case DrawMode::POINTS:
    glDrawArrays(GL_POINTS, 0, m_numVertices);
    break;
  case DrawMode::LINES:
    glDrawElements(GL_LINE_STRIP, m_numIndices, GL_UNSIGNED_INT, NULL);
    break;
  }
}
} // namespace ew
