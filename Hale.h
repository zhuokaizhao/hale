/*
  hale: support for minimalist scientific visualization
  Copyright (C) 2014  University of Chicago

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software. Permission is granted to anyone to
  use this software for any purpose, including commercial applications, and
  to alter it and redistribute it freely, subject to the following
  restrictions:

  1. The origin of this software must not be misrepresented; you must not
  claim that you wrote the original software. If you use this software in a
  product, an acknowledgment in the product documentation would be
  appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not be
  misrepresented as being the original software.

  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef HALE_INCLUDED
#define HALE_INCLUDED

#include <iostream>
#include <stdexcept>
#include <map>

/* This will include all the Teem headers at once */
#include <teem/meet.h>

/*
** We don't #define GLM_FORCE_RADIANS here because who knows what Hale users
** will expect or need. Hale's own source does use GLM_FORCE_RADIANS but
** that's in privateHale.h
*/
#include <glm/glm.hpp>

/*
** We want to restrict ourself to Core OpenGL, but have found that on at least
** one Linux System, using "#define GLFW_INCLUDE_GLCOREARB" caused things like
** glGetError and glViewport to not be defined.  Conversely, on that same
** linux machine, functions like glCreateShader and glShaderSource were not
** defined unless there was '#define GL_GLEXT_PROTOTYPES", even though
** <https://www.opengl.org/registry/ABI/> suggests that GL_GLEXT_PROTOTYPES is
** something one queries with #ifdef rather than #define'ing.  HEY: Some
** expertise here would be nice.
*/
#if defined(__APPLE_CC__)
#  define GLFW_INCLUDE_GLCOREARB
#else
#  define GL_GLEXT_PROTOTYPES
#endif
/* NB: on at least one Linux system that was missing GL/glcorearb.h,
   GLK followed advice from here:
   http://oglplus.org/oglplus/html/oglplus_getting_it_going.html and
   manually put glcorearb.h in /usr/include/GL, however the info here:
   https://www.opengl.org/registry/api/readme.pdf suggests that all
   those headers should be locally re-generated by a script */
#include <GLFW/glfw3.h>

namespace Hale {

typedef void (*ViewerRefresher)(void*);

/*
** enums.cpp: Various C enums are used to representing things with
** integers, and the airEnum provides mappings between strings and the
** corresponding integers
*/

/*
** viewerMode* enum
**
** The GUI modes that the viewer can be in. In Fov and Depth (distance from
** near to far adjusted), the look-from and look-at point are both fixed. The
** eye moves around a fixed look-at point in the Rotate and Vertigo
** modes. The eye and look-at points move together in the Translate modes.
*/
enum {
  viewerModeUnknown,        /*  0 */
  viewerModeNone,           /*  1: buttons released => no camera
                                interaction */
  viewerModeFov,            /*  2: standard "zoom" */
  viewerModeDepthScale,     /*  3: scale distance between near and far
                                clipping planes */
  viewerModeDepthTranslate, /*  4: shift near and far planes (together)
                                towards or away from eye point */
  viewerModeRotateUV,       /*  5: usual rotate (around look-at point) */
  viewerModeRotateU,        /*  6: rotate around horizontal axis */
  viewerModeRotateV,        /*  7: rotate around vertical axis */
  viewerModeRotateN,        /*  8: in-plane rotate (around at point) */
  viewerModeVertigo,        /*  9: fix at, move from, adjust fov: the effect
                                is direct control on amount of perspective
                                (aka dolly zoom, c.f. Hitchcock's Vertigo) */
  viewerModeTranslateUV,    /* 10: usual translate */
  viewerModeTranslateU,     /* 11: translate only horizontal */
  viewerModeTranslateV,     /* 12: translate only vertical */
  viewerModeDolly           /* 13: could be called TranslateN: translate from
                               *and* at along view direction */
};
extern airEnum *viewerMode;

enum {
  vertAttrIndxUnknown = -1, /* -1: (0 is a valid index) */
  vertAttrIndxXYZ,          /*  0: XYZ position */
  vertAttrIndxXYZW,         /*  1: XYZW position */
  vertAttrIndxNorm,         /*  2: 3-vector normal */
  vertAttrIndxRGB,          /*  3: RGB color */
  vertAttrIndxRGBA,         /*  4: RGBA color */
};
extern airEnum *vertAttrIndx;

enum {
  finishingStatusUnknown,   /* 0 */
  finishingStatusNot,       /* 1: we're still running */
  finishingStatusOkay,      /* 2: we're quitting gracefully */
  finishingStatusError      /* 3: we're exiting with error */
};
extern airEnum *finishingStatus;


/* utils.cpp */
extern bool finishing;
extern void init();
extern void done();
extern GLuint limnToGLPrim(int type);
extern void glErrorCheck(std::string whence, std::string context);
typedef struct {
  /* copy of the same enum value used for indexing into glEnumDesc */
  GLenum enumVal;
  /* string of the GLenum value, e.g. "GL_FLOAT", "GL_FLOAT_MAT4" */
  std::string enumStr;
  /* string for corresponding glsl type, e.g. "float", "mat4" */
  std::string glslStr;
} glEnumItem;
/* gadget to map GLenum values to something readable */
extern std::map<GLenum,glEnumItem> glEnumDesc;

/* Camera.cpp: like Teem's limnCamera but simpler: there is no notion of
   image-plane distance (because the range along U and V is wholly determined
   by fov and aspect), there is no control of right-vs-left handed
   coordinates (it is always right-handed: U increases to the right, V
   increases downward HEY HEY is that actually true, with use of
   glm::view()?), and clipNear and clipFar are always relative to look-at
   point. */
class Camera {
 public:
  explicit Camera(glm::vec3 from = glm::vec3(3.0f,4.0f,5.0f),
                  glm::vec3 at = glm::vec3(0.0f,0.0f,0.0f),
                  glm::vec3 up = glm::vec3(0.0f,0.0f,1.0f),
                  double fov = 0.8,
                  double aspect = 1.3333333,
                  double clipNear = -2,
                  double clipFar = 2,
                  bool orthographic = false);

  /* set/get verbosity level */
  void verbose(int);
  int verbose();

  /* set everything at once, as if at initialization */
  void init(glm::vec3 from, glm::vec3 at, glm::vec3 up,
            double fov, double aspect,
            double clipNear, double clipFar,
            bool orthographic);

  /* set/get world-space look-from, look-at, and pseudo-up */
  void from(glm::vec3); glm::vec3 from();
  void at(glm::vec3);   glm::vec3 at();
  void up(glm::vec3);   glm::vec3 up();

  /* make up orthogonal to at-from */
  void reup();

  /* setters, getters */
  void fov(double);        double fov();
  void aspect(double);     double aspect();
  void clipNear(double);   double clipNear();
  void clipFar(double);    double clipFar();
  void orthographic(bool); bool orthographic();

  /* the (world-to-)view and projection transforms
     determined by the above parameters */
  glm::mat4 view();
  glm::mat4 project();
  const float *viewPtr();
  const float *projectPtr();

  /* basis vectors of view space */
  glm::vec3 U();
  glm::vec3 V();
  glm::vec3 N();

 private:
  int _verbose;

  /* essential camera parameters */
  glm::vec3 _from, _at, _up;
  double _fov, _aspect, _clipNear, _clipFar;
  bool _orthographic;

  /* derived parameters */
  glm::vec3 _uu, _vv, _nn; // view-space basis
  glm::mat4 _view, _project;

  void updateView();
  void updateProject();
};

/* Viewer.cpp: Viewer contains and manages a GLFW window, including the
   camera that defines the view within the viewer.  We intercept all
   the events in order to handle how the camera is updated */
class Viewer {
 public:
  explicit Viewer(int width,  int height, const char *label);
  ~Viewer();

  /* the camera we update with user interactions */
  Camera camera;

  /* set/get verbosity level */
  void verbose(int);
  int verbose();

  /* set window title */
  void title();

  /* get width and height of window in screen-space, which is not always
     the same as dimensions of frame buffer (on high-DPI displays, the
     frame buffer size is larger than the nominal size of the window).
     There are no methods for setting width and height because that's
     handled by responding to GLFW resize events */
  int width();
  int height();

  /* set/get whether to fix the "up" vector during camera movements */
  void upFix(bool);
  bool upFix();

  /* set/get refresh callback, and its data */
  void refreshCB(ViewerRefresher cb);
  ViewerRefresher refreshCB();
  void refreshData(void *data);
  void *refreshData();

  /* swap render buffers in window */
  void bufferSwap();

  /* save current view to image */
  // int bufferSave(Nrrd *nrgba, Nrrd *ndepth);

 private:
  bool _button[2];     // true iff button (left:0, right:1) is down
  std::string _label;
  int _verbose;
  bool _upFix;
  int _mode;           // from Hale::viewerMode* enum
  ViewerRefresher _refreshCB;
  void * _refreshData;

  int _pixDensity,
    _widthScreen, _heightScreen,
    _widthBuffer, _heightBuffer;
  double _lastX, _lastY; // last clicked position, in screen space

  GLFWwindow *_window; // the window we manage
  static void cursorPosCB(GLFWwindow *gwin, double xx, double yy);
  static void framebufferSizeCB(GLFWwindow *gwin, int newWidth, int newHeight);
  static void keyCB(GLFWwindow *gwin, int key, int scancode, int action, int mods);
  static void windowCloseCB(GLFWwindow *gwin);
  static void windowRefreshCB(GLFWwindow *gwin);
  static void mouseButtonCB(GLFWwindow *gwin, int button, int action, int mods);

  void shapeUpdate();
};

/*
** Program.cpp: a GLSL shader program contains shader objects for vertex and
** fragment shaders (can easily add a geometry shader when needed)
*/

class Program {
 public:
  explicit Program(const char *vertFname, const char *fragFname);
  ~Program();
  void compile();
  void bindAttribute(GLuint idx, const GLchar *name);
  void link();
  void use();
  // will add more of these as needed
  void uniform(std::string, glm::vec3);
  void uniform(std::string, glm::mat4);
 private:
  std::map<std::string,GLint> _uniformLocation;
  std::map<std::string,glEnumItem> _uniformType;
  GLint _vertId, _fragId, _progId;
  GLchar *_vertCode, *_fragCode;
};

} // namespace Hale

#endif /* HALE_INCLUDED */
