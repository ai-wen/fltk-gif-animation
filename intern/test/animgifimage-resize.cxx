//
//  Test program for Fl_Anim_GIF_Image::copy().
//
#include <FL/Fl_Anim_GIF_Image.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <cstdio>

static Fl_Anim_GIF_Image *orig = 0;
static bool draw_grid = true;

class Canvas : public Fl_Box {
  typedef Fl_Box Inherited;
public:
  Canvas(int x_, int y_, int w_, int h_) :
    Inherited(x_, y_, w_, h_) {}
  virtual void draw() {
    if (draw_grid) {
      // draw a transparency grid as background
      static const Fl_Color C1 = fl_rgb_color(0xcc, 0xcc, 0xcc);
      static const Fl_Color C2 = fl_rgb_color(0x88, 0x88, 0x88);
      static const int SZ = 8;
      for (int y = 0; y < h(); y += SZ) {
        for (int x = 0; x < w(); x += SZ) {
          fl_color(x%(SZ * 2) ? y%(SZ * 2) ? C1 : C2 : y%(SZ * 2) ? C2 : C1);
          fl_rectf(x, y, 32, 32);
        }
      }
    }
    // draw the current image frame over the grid
    Inherited::draw();
  }
  void do_resize(int W_, int H_) {
    if (image() && (image()->w() != W_ || image()->h() != H_)) {
      Fl_Anim_GIF_Image *animgif = (Fl_Anim_GIF_Image *)image();
      animgif->stop();
      image(0);
      // delete already copied images
      if (animgif != orig ) {
        delete animgif;
      }
      Fl_Anim_GIF_Image *copied = (Fl_Anim_GIF_Image *)orig->copy(W_, H_);
      if (!copied->valid()) { // check success of copy
        Fl::warning("Fl_Anim_GIF_Image::copy() %d x %d failed", W_, H_);
      }
      else {
        printf("resized to %d x %d\n", copied->w(), copied->h());
      }
      copied->canvas(this, Fl_Anim_GIF_Image::Start |
                     Fl_Anim_GIF_Image::DontResizeCanvas);
      copied->start();
    }
    window()->cursor(FL_CURSOR_DEFAULT);
  }
  static void do_resize_cb(void *d_) {
    Canvas *c = (Canvas *)d_;
    c->do_resize(c->w(), c->h());
  }
  virtual void resize(int x_, int y_, int w_, int h_) {
    Inherited::resize(x_, y_, w_, h_);
    // decouple resize event from actual resize operation
    // to avoid lockups..
    Fl::remove_timeout(do_resize_cb, this);
    Fl::add_timeout(0.1, do_resize_cb, this);
    window()->cursor(FL_CURSOR_WAIT);
  }
};

int main(int argc_, char *argv_[]) {
  // setup play parameters from args
  const char *fileName = 0;
  bool bilinear = false;
  bool optimize = false;
  bool uncache = false;
  for (int i = 1; i < argc_; i++) {
    if (!strcmp(argv_[i], "-b")) // turn bilinear scaling on
      bilinear = true;
    else if (!strcmp(argv_[i], "-m")) // turn optimize on
      optimize = true;
    else if (!strcmp(argv_[i], "-g")) // disable grid
      draw_grid = false;
    else if (!strcmp(argv_[i], "-u")) // uncache
      uncache = true;
    else if (argv_[i][0] != '-' && !fileName) {
      fileName = argv_[i];
    }
  }
  if (!fileName) {
    fprintf(stderr, "Test program for animated copy.\n");
    fprintf(stderr, "Usage: %s fileName [-b] [-m] [-g] [-u]\n", argv_[0]);
    exit(0);
  }
  Fl_Anim_GIF_Image::min_delay = 0.1; // set a minumum delay for playback

  Fl_Double_Window win(640, 480);

  // prepare a canvas for the animation
  // (we want to show it in the center of the window)
  Canvas canvas(0, 0, win.w(), win.h());
  win.resizable(win);

  win.end();
  win.show();

  // create/load the animated gif and start it immediately.
  // We use the 'DontResizeCanvas' flag here to tell the
  // animation not to change the canvas size (which is the default).
  int flags = Fl_Anim_GIF_Image::Start | Fl_Anim_GIF_Image::DontResizeCanvas;
  if (optimize) {
    flags |= Fl_Anim_GIF_Image::OptimizeMemory;
    printf("Using memory optimization (if image supports)\n");
  }
  orig = new Fl_Anim_GIF_Image(/*name_=*/ fileName,
                             /*canvas_=*/ &canvas,
                              /*flags_=*/ flags );

  // check if loading succeeded
  printf("%s: valid: %d frames: %d uncache: %d\n",
    orig->name(), orig->valid(), orig->frames(), orig->frame_uncache());
  if (orig->valid()) {
    win.copy_label(fileName);

    // print information about image optimization
    int n = 0;
    for (int i = 0; i < orig->frames(); i++) {
      if (orig->frame_x(i) != 0 || orig->frame_y(i) != 0) n++;
    }
    printf("image has %d optimized frames\n", n);

    if (bilinear) {
      Fl_RGB_Image::RGB_scaling(FL_RGB_SCALING_BILINEAR);
      printf("Using bilinear scaling - can be slow!\n");
      // NOTE: this is *really* slow. Scaling the TrueColor test image
      //       to full HD desktop takes about 45 seconds!
    }
    orig->frame_uncache(uncache);
    if (uncache) {
      printf("Caching disabled - watch cpu load!\n");
    }

    // set initial size to fit into window
    double ratio = orig->valid() ? (double)orig->w() / orig->h() : 1;
    int W = win.w() - 40;
    int H = (double)W / ratio;
    printf("original size: %d x %d\n", orig->w(), orig->h());
    win.size(W, H);

    return Fl::run();
  }
}
