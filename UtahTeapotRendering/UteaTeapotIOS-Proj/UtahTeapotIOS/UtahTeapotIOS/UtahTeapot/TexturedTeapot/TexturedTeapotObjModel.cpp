//--------------------------------------------------------------------------------
// TexturedTeapotObjModel.cpp
// Render a textured utah teapot
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Include files
//--------------------------------------------------------------------------------
#include "TexturedTeapotObjModel.h"

//--------------------------------------------------------------------------------
// Teapot model data
//--------------------------------------------------------------------------------
#include "matrix4.h"
#include "Geometry.h"
#include "Texture.h"
#include "geometrymaker.h"

//Note TARGET_OS_MAC includes any apple OS,TARGET_OS_IPHONE includes any device/simulator run on iOS
#if defined(__APPLE__) && defined(__MACH__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX   //定位OSX系统的macro
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#elif TARGET_OS_IPHONE //用于定位IPHONE系统的macro
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#endif
#endif

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>


vector<VertexPNX> loadObj(){
    
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    
    
    std::string warn;
    std::string err;
    
    std::string filename = "teapot.obj";
    
    std::ifstream ifs(GetBundleFileName(filename.c_str()));
    
    if (ifs.fail()) {
        std::cerr << "file not found." << std::endl;
        return {};
    }
    
    //tinyobj::MaterialFileReader mtlReader("../../models/");
    
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &ifs);
    
    if (!warn.empty()) {
        std::cout << "WARN: " << warn << std::endl;
    }
    
    if (!err.empty()) {
        std::cerr << err << std::endl;
    }
    
    if (!ret) {
        std::cerr << "Failed to parse .obj" << std::endl;
        return {};
    }
    
    vector<float> vertices = attrib.vertices;
    vector<float> normals = attrib.normals;
    vector<float> texcoords = attrib.texcoords;
    vector<tinyobj::index_t> indices = shapes[0].mesh.indices;
    vector<VertexPNX> vtxVertcies;
    for(tinyobj::index_t it:indices){
        int vIdx = it.vertex_index;
        Cvec3 vertex(vertices[3*vIdx],vertices[3*vIdx+1],vertices[3*vIdx+2]);
        int nIdx = it.normal_index;
        Cvec3 normal(normals[3*nIdx],normals[3*nIdx+1],normals[3*nIdx+2]);
        int tIdx = it.texcoord_index;
        Cvec2 texture(texcoords[2*tIdx],texcoords[2*tIdx+1]);
        VertexPNX vpnx(vertex,normal,texture);
        vtxVertcies.push_back(vpnx);
    }
    
    
    return vtxVertcies;
    
}


//--------------------------------------------------------------------------------
// Ctor
//--------------------------------------------------------------------------------
TexturedTeapotObjModel::TexturedTeapotObjModel() {}

//--------------------------------------------------------------------------------
// Dtor
//--------------------------------------------------------------------------------
TexturedTeapotObjModel::~TexturedTeapotObjModel(){
    Unload();
}

/**
 * Report type of teapot we are rendering. This is the only place
 * to decide what kind of teapot to render.
 *
 * @return
 *   GL_TEXTURE_CUBE_MAP if you want to render cubemaped teapot
 *   GL_TEXTURE_2D if just to render a 2D textured teapot
 *   GL_INVALID_VALUE no texture for teapot
 */
GLint TexturedTeapotObjModel::GetTextureType(void) {
    return
        GL_TEXTURE_CUBE_MAP;
//    GL_TEXTURE_2D;
    //            GL_INVALID_VALUE;
}

void TexturedTeapotObjModel::Init() {
    // Settings
    glFrontFace(GL_CCW);
    
    vector<VertexPNX> noIdxVertices = loadObj();
    
    num_vertices_ = (int)noIdxVertices.size();
    
    geometry_.reset(new Geometry(&noIdxVertices[0], NULL, num_vertices_, 0));
    
    // Load shader
    teapotShaderState_.reset(new TexturedTeapotShaderState());
    
    
    GLint type = GetTextureType();
    
//    std::vector<std::string> textures {
//        std::string("./textures/posx.jpg"),  // GL_TEXTURE_CUBE_MAP_POSITIVE_X
//        std::string("./textures/negx.jpg"),   // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
//        std::string("./textures/posy.jpg"),    // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
//        std::string("./textures/negy.jpg"), // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
//        std::string("./textures/posz.jpg"),  // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
//        std::string("./textures/negz.jpg")    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
//    };
//
//    if(type == GL_TEXTURE_2D) {
//        textures[0] = std::string("./textures/portrait.png");
//    }
    
    std::vector<std::string> textures {
        std::string(GetBundleFileName("posx.jpg")),  // GL_TEXTURE_CUBE_MAP_POSITIVE_X
        std::string(GetBundleFileName("negx.jpg")),   // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
        std::string(GetBundleFileName("posy.jpg")),    // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
        std::string(GetBundleFileName("negy.jpg")), // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
        std::string(GetBundleFileName("posz.jpg")),  // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
        std::string(GetBundleFileName("negz.jpg"))    // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };
    
    if(type == GL_TEXTURE_2D) {
        textures[0] = std::string(GetBundleFileName("greenleaves.png"));
    }
    
    texObj_ = Texture::Create(type, textures);
    assert(texObj_);
    
    std::vector<std::string> samplers;
    std::vector<GLint> units;
    texObj_->GetActiveSamplerInfo(samplers, units);
    for(size_t idx = 0; idx < samplers.size(); idx++) {
        GLint sampler = glGetUniformLocation(teapotShaderState_->program,
                                             samplers[idx].c_str());
        glUniform1i(sampler, units[idx]);
    }
    
    texObj_->Activate();
    
    if(!vao_)
        glGenVertexArrays( 1, &vao_ );
    
    glBindVertexArray(vao_);
    
    // Bind the VBO
    glBindBuffer(GL_ARRAY_BUFFER, geometry_->vbo);
    
    int32_t iStride = sizeof(VertexPNX);
    // Pass the vertex data
    glVertexAttribPointer(ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, iStride,
                          BUFFER_OFFSET(0));
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    
    glVertexAttribPointer(ATTRIB_NORMAL, 3, GL_FLOAT, GL_FALSE, iStride,
                          BUFFER_OFFSET(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(ATTRIB_NORMAL);
    
    glVertexAttribPointer(ATTRIB_UV, 2, GL_FLOAT, GL_FALSE, iStride,
                          BUFFER_OFFSET(3 * sizeof(GLfloat)+ 3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(ATTRIB_UV);
    
    glBindVertexArray(0);
    

    mat_local_model_ = Matrix4::makeTranslation(Cvec3(0, 0, -30.f));
    //mat_local_model_ =  mat_local_model_ * Matrix4::makeXRotation(-70);
        
    mat_view_ = Matrix4::makeTranslation(Cvec3(0,0.0f,4.0f));
    mat_view_ = inv(mat_view_);
    
    UpdateViewport();
}

void TexturedTeapotObjModel::UpdateViewport() {
    // Init Projection matrices
    int32_t viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    const float CAM_NEAR = -0.1f;
    const float CAM_FAR = -10000.f;
    
    
    float fov = 60;
    float aspect = static_cast<float>(viewport[2]) / static_cast<float>(viewport[3]);
    mat_projection_ =Matrix4::makeProjection(fov, aspect, CAM_NEAR, CAM_FAR);
}

void TexturedTeapotObjModel::Unload() {
    
}

void TexturedTeapotObjModel::Update(double time) {
    if (motionControl_) {
        motionControl_->Update();
        //mat_view_ = appCamera_->GetTransformMatrix() * mat_view_ *
        //appCamera_->GetRotationMatrix();
        //mat_view_ =  mat_view_ * appCamera_->GetRotationMatrix();
        
        mat_motion_matrix_ =  motionControl_->GetRotationMatrix();
    }
}


void TexturedTeapotObjModel::SetMotionControl(std::shared_ptr<MotionControl> mControl){
    motionControl_ = mControl;
}

void TexturedTeapotObjModel::Render(float r, float g, float b) {
    
    //mat_model_ = mat_model_ * Matrix4::makeYRotation(0.5);
    mat_model_ = mat_local_model_ * mat_motion_matrix_;
    
    Matrix4 mat_mv = mat_view_ * mat_model_;
    // Feed Projection and Model View matrices to the shaders
    Matrix4 mat_vp = mat_projection_ * mat_mv;
    
    // Bind the IB
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry_->ibo);
    
    glUseProgram(teapotShaderState_->program);
    
    TEAPOT_MATERIALS material = {
        {1.0, 0.5, 0.5}, {1.0f, 1.0f, 1.0f, 10.f}, {0.1f, 0.1f, 0.1f}, };
    
    // Update uniforms
    glUniform4f(teapotShaderState_->material_diffuse_, material.diffuse_color[0],
                material.diffuse_color[1], material.diffuse_color[2], 1.f);
    
    glUniform4f(teapotShaderState_->material_specular_, material.specular_color[0],
                material.specular_color[1], material.specular_color[2],
                material.specular_color[3]);
    
    // using glUniform3fv here was troublesome
    
    glUniform3f(teapotShaderState_->material_ambient_, material.ambient_color[0],
                material.ambient_color[1], material.ambient_color[2]);
    
    GLfloat glmatrix[16];
    mat_vp.writeToColumnMajorMatrix(glmatrix);
    glUniformMatrix4fv(teapotShaderState_->matrix_projection_, 1, GL_FALSE,
                       glmatrix);
    
    GLfloat glmatrix2[16];
    mat_mv.writeToColumnMajorMatrix(glmatrix2);
    glUniformMatrix4fv(teapotShaderState_->matrix_view_, 1, GL_FALSE, glmatrix2);
    
    GLfloat glmatrix3[16];
    mat_model_.writeToColumnMajorMatrix(glmatrix3);
    glUniformMatrix4fv(teapotShaderState_->matrix_model_, 1, GL_FALSE, glmatrix3);
    
    glUniform3f(teapotShaderState_->light0_, 200.f, 100.f, -600.f);
    
    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices_);
    glBindVertexArray(0);

}

