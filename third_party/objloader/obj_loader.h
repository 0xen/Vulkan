/******************************************************************************
 * Copyright 1998-2018 NVIDIA Corp. All Rights Reserved.
 *****************************************************************************/

#pragma once
#include "glm/glm.hpp"
#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <iostream>
#include "tiny_obj_loader.h"
#include <unordered_map>
#include <vector>

// Structure holding the material
struct MatrialObj
{
  glm::vec3 ambient       = glm::vec3(0.1f, 0.1f, 0.1f);
  glm::vec3 diffuse       = glm::vec3(0.7f, 0.7f, 0.7f);
  glm::vec3 specular      = glm::vec3(1.0f, 1.0f, 1.0f);
  glm::vec3 transmittance = glm::vec3(0.0f, 0.0f, 0.0f);
  glm::vec3 emission      = glm::vec3(0.0f, 0.0f, 0.10);
  float     shininess     = 0.f;
  float     ior           = 1.0f;  // index of refraction
  float     dissolve      = 1.f;   // 1 == opaque; 0 == fully transparent
      // illumination model (see http://www.fileformat.info/format/material/)
  int illum     = 0;
  int textureID = -1;
  int metalicTextureID = -1;
  int roughnessTextureID = -1;
  int normalTextureID = -1;
  int cavityTextureID = -1;
  int aoTextureID = -1;
  int heightTextureID = -1;
  int n[2];//Padding
};

template <class TVert>
class ObjLoader
{
public:
  void loadModel(const std::string& filename);

  std::vector<TVert>       m_vertices;
  std::vector<uint32_t>    m_indices;
  std::vector<MatrialObj>  m_materials;
  std::vector<std::string> m_textures;
};

//-----------------------------------------------------------------------------
// Extract the directory component from a complete path.
//
#ifdef WIN32
#define CORRECT_PATH_SEP "\\"
#define WRONG_PATH_SEP '/'
#else
#define CORRECT_PATH_SEP "/"
#define WRONG_PATH_SEP '\\'
#endif

static inline std::string get_path(const std::string& file)
{
  std::string dir;
  size_t      idx = file.find_last_of("\\/");
  if(idx != std::string::npos)
    dir = file.substr(0, idx);
  if(!dir.empty())
  {
    dir += CORRECT_PATH_SEP;
  }
  return dir;
}

template <class TVert>
void ObjLoader<TVert>::loadModel(const std::string& filename)
{
  tinyobj::attrib_t                attrib;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;
  std::string                      err;

  std::string materialPath = get_path(filename);

  if(!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str(), materialPath.c_str()))
  {
    std::cerr << "Cannot load: " << filename << std::endl;
    throw std::runtime_error(err);
  }

  m_vertices.reserve(shapes[0].mesh.indices.size());

  // Collecting the material in the scene
  for(const auto& material : materials)
  {
    MatrialObj m;
    m.ambient  = glm::vec3(material.ambient[0], material.ambient[1], material.ambient[2]);
    m.diffuse  = glm::vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
    m.specular = glm::vec3(material.specular[0], material.specular[1], material.specular[2]);
    m.emission = glm::vec3(material.emission[0], material.emission[1], material.emission[2]);
    m.transmittance =
        glm::vec3(material.transmittance[0], material.transmittance[1], material.transmittance[2]);
    m.dissolve = material.dissolve;
    m.ior      = material.ior;
    m.illum    = material.illum;
	m.shininess = material.shininess;

    m.textureID = -1;
    m.metalicTextureID = -1;
    m.roughnessTextureID = -1;
    m.normalTextureID = -1;
    m.cavityTextureID = -1;
    m.aoTextureID = -1;
    m.heightTextureID = -1;

    if(!material.diffuse_texname.empty())
    {
      m_textures.push_back(material.diffuse_texname);
      m.textureID = static_cast<int>(m_textures.size()) - 1;
    }
	if (!material.reflection_texname.empty())
	{
		m_textures.push_back(material.reflection_texname);
		m.metalicTextureID = static_cast<int>(m_textures.size()) - 1;
	}
	if (!material.normal_texname.empty())
	{
		m_textures.push_back(material.normal_texname);
		m.normalTextureID = static_cast<int>(m_textures.size()) - 1;
	}
	if (!material.specular_highlight_texname.empty())
	{
		m_textures.push_back(material.specular_highlight_texname);
		m.roughnessTextureID = static_cast<int>(m_textures.size()) - 1;
	}

    m_materials.emplace_back(m);
  }

  // If there were none, add a default
  if(m_materials.empty())
    m_materials.emplace_back(MatrialObj());

  for(const auto& shape : shapes)
  {
    m_vertices.reserve(shape.mesh.indices.size() + m_vertices.size());
    m_indices.reserve(shape.mesh.indices.size() + m_indices.size());

    uint32_t faceID    = 0;
    int      index_cnt = 0;

    for(const auto& index : shape.mesh.indices)
    {
      TVert  vertex = {};
      float* vp     = &attrib.vertices[3 * index.vertex_index];
      vertex.pos    = {*(vp + 0), *(vp + 1), *(vp + 2)};

      if(!attrib.normals.empty() && index.normal_index >= 0)
      {
        float* np  = &attrib.normals[3 * index.normal_index];
        vertex.nrm = {*(np + 0), *(np + 1), *(np + 2)};
      }

      if(!attrib.texcoords.empty() && index.texcoord_index >= 0)
      {
        float* tp       = &attrib.texcoords[2 * index.texcoord_index + 0];
        vertex.texCoord = {*tp, 1.0f - *(tp + 1)};
      }

      if(!attrib.colors.empty())
      {
        float* vc    = &attrib.colors[3 * index.vertex_index];
        vertex.color = {*(vc + 0), *(vc + 1), *(vc + 2)};
      }

      vertex.matID = shape.mesh.material_ids[faceID];
      if(vertex.matID < 0 || vertex.matID >= m_materials.size())
        vertex.matID = 0;
      index_cnt++;
      if(index_cnt >= 3)
      {
        ++faceID;
        index_cnt = 0;
      }

      m_vertices.push_back(vertex);
      m_indices.push_back(static_cast<int>(m_indices.size()));
    }
  }

  // Compute normal when no normal were provided.
  if(attrib.normals.empty())
  {
    for(size_t i = 0; i < m_indices.size(); i += 3)
    {
      TVert& v0 = m_vertices[m_indices[i + 0]];
      TVert& v1 = m_vertices[m_indices[i + 1]];
      TVert& v2 = m_vertices[m_indices[i + 2]];

      glm::vec3 n = glm::normalize(glm::cross((v1.pos - v0.pos), (v2.pos - v0.pos)));
      v0.nrm      = n;
      v1.nrm      = n;
      v2.nrm      = n;
    }
  }
}
