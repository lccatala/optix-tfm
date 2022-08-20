// ======================================================================== //
// Copyright 2018-2019 Ingo Wald                                            //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "SampleRenderer.h"

// our helper library for window handling
#include "glfWindow/GLFWindow.h"
#include <GL/gl.h>
#include <unordered_map>

/*! \namespace osc - Optix Siggraph Course */
namespace osc {

  struct SampleWindow : public GLFCameraWindow
  {
    SampleWindow(const std::string &title,
                 const Model *model,
                 const Camera &camera,
                 const float worldScale,
                 const int width,
                 const int height)
      : GLFCameraWindow(title,camera.from,camera.at,camera.up,worldScale, width, height),
        sample(model)
    {
      sample.setCamera(camera);
    }

    void writeAccelBuildTime(const std::string& filename)
    {
      std::ofstream outputFile;
      outputFile.open(filename+"-blasbuildtime.txt", std::ios::out | std::ios::app);
      outputFile << sample.getBLASBuildTime().count() << '\n';
      outputFile.close();
      
      outputFile.open(filename + "-tlasbuildtime.txt", std::ios::out | std::ios::app);
      outputFile << sample.getTLASBuildTime().count() << '\n';
      outputFile.close();
    }
    std::chrono::microseconds getAccelBuildTime() { return sample.getAccelBuildTime(); }
    
    virtual void render() override
    {
      if (cameraFrame.modified) {
        sample.setCamera(Camera{ cameraFrame.get_from(),
                                 cameraFrame.get_at(),
                                 cameraFrame.get_up() });
        cameraFrame.modified = false;
      }
      sample.render();
    }
    
    virtual void draw() override
    {
      sample.downloadPixels(pixels.data());
      if (fbTexture == 0)
        glGenTextures(1, &fbTexture);
      
      glBindTexture(GL_TEXTURE_2D, fbTexture);
      GLenum texFormat = GL_RGBA;
      GLenum texelType = GL_UNSIGNED_BYTE;
      glTexImage2D(GL_TEXTURE_2D, 0, texFormat, fbSize.x, fbSize.y, 0, GL_RGBA,
                   texelType, pixels.data());

      glDisable(GL_LIGHTING);
      glColor3f(1, 1, 1);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, fbTexture);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      
      glDisable(GL_DEPTH_TEST);

      glViewport(0, 0, fbSize.x, fbSize.y);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0.f, (float)fbSize.x, 0.f, (float)fbSize.y, -1.f, 1.f);

      glBegin(GL_QUADS);
      {
        glTexCoord2f(0.f, 0.f);
        glVertex3f(0.f, 0.f, 0.f);
      
        glTexCoord2f(0.f, 1.f);
        glVertex3f(0.f, (float)fbSize.y, 0.f);
      
        glTexCoord2f(1.f, 1.f);
        glVertex3f((float)fbSize.x, (float)fbSize.y, 0.f);
      
        glTexCoord2f(1.f, 0.f);
        glVertex3f((float)fbSize.x, 0.f, 0.f);
      }
      glEnd();
    }
    
    virtual void resize(const vec2i &newSize) 
    {
      fbSize = newSize;
      sample.resize(newSize);
      pixels.resize(newSize.x*newSize.y);
    }

    vec2i                 fbSize;
    GLuint                fbTexture {0};
    SampleRenderer        sample;
    std::vector<uint32_t> pixels;
  };
  
  
  /*! main entry point to this example - initially optix, print hello
    world, then exit */
  extern "C" int main(int ac, char **av)
  {
    try {
      int windowWidth = 1920;
      int windowHeight = 1080;
      std::string modelName = "iscv2";
      if (ac >= 4)
      {
        windowWidth = atoi(av[1]);
        windowHeight = atoi(av[2]);
        modelName = std::string(av[3]);
      }
      std::unordered_map<std::string, vec3f> modelCameraOrigins = { 
        {"bmw", {-500.0f, 150.0f, -800.0f} },
        {"hairball", {10.0f, 15.0f, -10.0f} },
        {"sponza", {-1293.07f, 154.681f, -0.7304f} },
        {"triangle", {-1293.07f, 154.681f, -0.7304f} },
      };
      //std::string modelName = "human";
      //std::string modelName = "iscv2";
      //std::string modelName = "sponza";
      //std::string modelName = "bmw";
      //std::string modelName = "gallery";
      Model *model = loadOBJ(
#ifdef _WIN32
      // on windows, visual studio creates _two_ levels of build dir
      // (x86/Release)
      "../../models/" + modelName + "/" + modelName + ".obj"
#else
      // on linux, common practice is to have ONE level of build dir
      // (say, <project>/build/)...
      "../models/sponza.obj"
#endif
                             );
      vec3f cameraOrigin = { -1293.07f, 154.681f, -0.7304f };
      if (modelCameraOrigins.count(modelName) != 0)
        cameraOrigin = modelCameraOrigins.at(modelName);
      Camera camera = { /*from*/cameraOrigin,
                        /* at */model->bounds.center(),
                        /* up */vec3f(0.f,1.f,0.f) };
      // something approximating the scale of the world, so the
      // camera knows how much to move for any given user interaction:
      const float worldScale = length(model->bounds.span());

      SampleWindow *window = new SampleWindow(modelName + "-shadows",
                                              model,camera,worldScale, windowWidth, windowHeight);
      window->writeAccelBuildTime("optix-" + std::to_string(windowWidth) + "x" + std::to_string(windowHeight) + modelName + "-shadows");
      //exit(0);
      window->run();
      
    } catch (std::runtime_error& e) {
      std::cout << GDT_TERMINAL_RED << "FATAL ERROR: " << e.what()
                << GDT_TERMINAL_DEFAULT << std::endl;
	  std::cout << "Did you forget to copy sponza.obj and sponza.mtl into your optix7course/models directory?" << std::endl;
	  exit(1);
    }
    return 0;
  }
  
} // ::osc
