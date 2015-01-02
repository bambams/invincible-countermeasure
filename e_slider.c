#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>

#include "m_config.h"

#include "g_header.h"
#include "e_slider.h"
#include "m_globvars.h"
#include "m_input.h"
#include "i_header.h"

extern struct fontstruct font [FONTS];
extern ALLEGRO_DISPLAY* display;

void fix_slider_pos(struct sliderstruct* sl);
void slider_moved_to_pos(struct sliderstruct* sl, int new_pos);

char down_arrow [2];


enum
{
POINTER_BUTTON_LESS,
POINTER_BUTTON_MORE,
POINTER_TRACK_LESS,
POINTER_TRACK_MORE,
POINTER_SLIDER,
POINTER_OUTSIDE
};



// can call this anytime, e.g. when the minimim or maximum values change but nothing else does. May result in no change.
// slider_represents_size is the range of values represented by the slider - e.g. for a scrollbar, this is the number of visible lines. For other sliders this is probably 1.
// this function assumes track_length is reasonable (may produce odd results if very small and crash if zero)
void init_slider(struct sliderstruct* sl, int* value_pointer, int dir, int value_min, int value_max, int total_length, int button_increment, int track_increment, int slider_represents_size, int x, int y, int thickness, int colour, int hidden_if_unused)
{

 down_arrow [0] = 127;
 down_arrow [1] = 0;

// set up the basics
 sl->dir = dir;
 sl->value_min = value_min;
 sl->value_max = value_max;
 sl->button_increment = button_increment;
 sl->track_increment = track_increment;
 sl->value = *value_pointer;
 sl->value_pointer = value_pointer;
 sl->total_length = total_length;
 sl->highlighted = POINTER_OUTSIDE;
 sl->hidden_if_unused = hidden_if_unused;
 sl->hidden = sl->hidden_if_unused;

 sl->x1 = x; // x and y are absolute values (for the screen)
 sl->y1 = y;
 if (dir == SLIDEDIR_VERTICAL)
 {
  sl->x2 = x + thickness;
  sl->y2 = y + total_length;
 }
  else
  {
   sl->x2 = x + total_length;
   sl->y2 = y + thickness ;
  }

// now derive other properties
 sl->track_length = total_length - (SLIDER_BUTTON_SIZE * 2);

 int range = value_max - value_min;

// sl->slider_size = (float) (slider_represents_size * sl->track_length) / range;
 sl->slider_size = (float) (slider_represents_size * sl->track_length) / (range + slider_represents_size);
// fprintf(stdout, "\nSlider size %i (repr %i track_length %i range %i value_min %i value_max %i)", sl->slider_size, slider_represents_size, sl->track_length, range, value_min, value_max);
// wait_for_space();
 if (sl->slider_size < SLIDER_MIN_SLIDER_SIZE)
  sl->slider_size = SLIDER_MIN_SLIDER_SIZE;

 sl->value_per_pixel = (float) range / (sl->track_length - sl->slider_size); // v_p_p is float

// sl->slider_size = (float) slider_represents_size / sl->value_per_pixel;

/*
 sl->track_length = total_length - (SLIDER_BUTTON_SIZE * 2);
 sl->value_per_pixel = (float) range / sl->track_length; // v_p_p is float

 sl->slider_size = (float) slider_represents_size / sl->value_per_pixel;
 if (sl->slider_size < SLIDER_MIN_SLIDER_SIZE)
  sl->slider_size = SLIDER_MIN_SLIDER_SIZE;*/

 sl->slider_pos = (sl->value - sl->value_min) / sl->value_per_pixel;

 sl->hold_type = SLIDER_HOLD_NONE;
 sl->hold_delay = 0;

 sl->colour = colour;

 fix_slider_pos(sl);

}

// this makes sure sl->slider_pos is within bounds.
void fix_slider_pos(struct sliderstruct* sl)
{

 if (sl->slider_pos < 0)
  sl->slider_pos = 0;
 if (sl->slider_pos > (sl->track_length - sl->slider_size))
  sl->slider_pos = (sl->track_length - sl->slider_size);
}


// call this when slider's up or down button clicked
void slider_button_or_track_clicked(struct sliderstruct* sl, int change)
{
 sl->value += change;

 if (sl->value < sl->value_min)
  sl->value = sl->value_min;
 if (sl->value > sl->value_max)
  sl->value = sl->value_max;

 *(sl->value_pointer) = sl->value;

 sl->slider_pos = (sl->value - sl->value_min) / sl->value_per_pixel;

 fix_slider_pos(sl);

// fprintf(stdout, "\nvalue %i slider_pos %i track_length %i value_per_pixel %f slider_size %i", sl->value, sl->slider_pos, sl->track_length, sl->value_per_pixel, sl->slider_size);

}

// new_pos is the new sl->slider_pos, in pixels
// this function doesn't assume new_pos is within bounds (and fixes it if it isn't)
void slider_moved_to_value(struct sliderstruct* sl, int new_value)
{

 sl->value = new_value;

 if (sl->value < sl->value_min)
  sl->value = sl->value_min;
 if (sl->value > sl->value_max)
  sl->value = sl->value_max;

 *(sl->value_pointer) = sl->value;

 sl->slider_pos = (sl->value - sl->value_min) / sl->value_per_pixel;

 fix_slider_pos(sl);

}


void slider_moved_to_pos(struct sliderstruct* sl, int new_pos)
{

 sl->slider_pos = new_pos;

 fix_slider_pos(sl);

// now we need to update the value:

// the +0.5 here is to avoid rounding down to next lowest int when the slider should be in maximum position
 sl->value = (float) sl->value_min + 0.5 + (sl->slider_pos * sl->value_per_pixel);// + sl->hold_offset_fine;

// fprintf(stdout, "(value:%i(%i-%i))", sl->value, sl->value_min, sl->value_max);

// because sl->value_per_pixel is float, we should do bounds checking in case of a rounding error:
 if (sl->value < sl->value_min)
  sl->value = sl->value_min;
 if (sl->value > sl->value_max)
  sl->value = sl->value_max;

 *(sl->value_pointer) = sl->value;

// fprintf(stdout, "(sl_pos1:%f)", sl->slider_pos);

 sl->slider_pos = (sl->value - sl->value_min) / sl->value_per_pixel;

// fprintf(stdout, "(sl_pos2:%f:value:%i:value_min:%i:value_max:%i:vpp:%f)", sl->slider_pos, sl->value, sl->value_min, sl->value_max, sl->value_per_pixel);

 fix_slider_pos(sl);

}

// this function turns slider sl into a thing that looks like a scrollbar on the screen (doesn't actually have to be used as a scrollbar, though)
// x_offset and y_offset are needed to translate the absolute position of the scrollbar to the position on the current target bitmap
// both should be zero if drawing to display or similar
void draw_scrollbar(struct sliderstruct* sl, int x_offset, int y_offset)
{

	if (sl->hidden == 1)
		return;

 float x1 = sl->x1 - x_offset + 0.5;
 float y1 = sl->y1 - y_offset + 0.5;
 float x2 = sl->x2 - x_offset - 0.5;
 float y2 = sl->y2 - y_offset - 0.5;

// first, draw background:
 al_draw_filled_rectangle(x1, y1, x2, y2, colours.base [sl->colour] [SHADE_MIN]);
// draw an outer border:
 al_draw_rectangle(x1, y1, x2, y2, colours.base [sl->colour] [SHADE_LOW], 1);

 if (sl->dir == SLIDEDIR_VERTICAL)
 {
// draw a button at each end:
  if (sl->highlighted == POINTER_BUTTON_LESS)
   al_draw_filled_rectangle(x1 + 1, y1 + 1, x2 - 1, y1 + SLIDER_BUTTON_SIZE, colours.base [sl->colour] [SHADE_LOW]);
  al_draw_rectangle(x1 + 1, y1 + 1, x2 - 1, y1 + SLIDER_BUTTON_SIZE, colours.base [sl->colour] [SHADE_MED], 1);
  al_draw_textf(font[FONT_BASIC].fnt, colours.base [sl->colour] [SHADE_MED], x1 + 9, y1 + 8, ALLEGRO_ALIGN_CENTRE, "^");
//  al_draw_line(x1 + 9, y1 + 7, x1 + 6, y1 + 12, colours.base [sl->colour] [SHADE_MED], 1);
//  al_draw_line(x1 + 9, y1 + 7, x1 + 12, y1 + 12, colours.base [sl->colour] [SHADE_MED], 1);
//  al_draw_triangle(x1 + 5, y1 + SLIDER_BUTTON_SIZE - 6, x1 + (SLIDER_BUTTON_SIZE/2) - 1, y1 + 5, x2 - 7, y1 + SLIDER_BUTTON_SIZE - 6, colours.base [sl->colour] [SHADE_MED], 1);
  if (sl->highlighted == POINTER_BUTTON_MORE)
   al_draw_filled_rectangle(x1 + 1, y2 - SLIDER_BUTTON_SIZE, x2 - 1, y2 - 1, colours.base [sl->colour] [SHADE_LOW]);
  al_draw_textf(font[FONT_BASIC].fnt, colours.base [sl->colour] [SHADE_MED], x1 + 9, y2 - SLIDER_BUTTON_SIZE + 1, ALLEGRO_ALIGN_CENTRE, "%s", down_arrow);
  al_draw_rectangle(x1 + 1, y2 - SLIDER_BUTTON_SIZE, x2 - 1, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

// draw slider:
   if (sl->highlighted == POINTER_SLIDER
    || sl->hold_type == SLIDER_HOLD_SLIDER)
   al_draw_filled_rectangle(x1 + 1, y1 + SLIDER_BUTTON_SIZE + sl->slider_pos, x2 - 1, y1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, colours.base [sl->colour] [SHADE_LOW]);
  al_draw_rectangle(x1 + 1, y1 + SLIDER_BUTTON_SIZE + sl->slider_pos, x2 - 1, y1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, colours.base [sl->colour] [SHADE_MED], 1);

 }
  else // SLIDEDIR_HORIZONTAL
  {
// draw a button at each end:
   if (sl->highlighted == POINTER_BUTTON_LESS)
    al_draw_filled_rectangle(x1 + 1, y1 + 1, x1 + SLIDER_BUTTON_SIZE, y2 - 1, colours.base [sl->colour] [SHADE_LOW]);
   al_draw_textf(font[FONT_BASIC].fnt, colours.base [sl->colour] [SHADE_MED], x1 + 9, y1 + 5, ALLEGRO_ALIGN_CENTRE, "<");
   al_draw_rectangle(x1 + 1, y1 + 1, x1 + SLIDER_BUTTON_SIZE, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

   if (sl->highlighted == POINTER_BUTTON_MORE)
    al_draw_filled_rectangle(x2 - SLIDER_BUTTON_SIZE, y1 + 1, x2 - 1, y2 - 1, colours.base [sl->colour] [SHADE_LOW]);
   al_draw_textf(font[FONT_BASIC].fnt, colours.base [sl->colour] [SHADE_MED], x2 - 9, y1 + 5, ALLEGRO_ALIGN_CENTRE, ">");
   al_draw_rectangle(x2 - SLIDER_BUTTON_SIZE, y1 + 1, x2 - 1, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

//   al_draw_rectangle(x1 + 1, y1 + 1, x1 + SLIDER_BUTTON_SIZE, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

// draw slider:
   if (sl->highlighted == POINTER_SLIDER
    || sl->hold_type == SLIDER_HOLD_SLIDER)
    al_draw_filled_rectangle(x1 + SLIDER_BUTTON_SIZE + sl->slider_pos, y1 + 1, x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, y2 - 1, colours.base [sl->colour] [SHADE_LOW]);
   al_draw_rectangle(x1 + SLIDER_BUTTON_SIZE + sl->slider_pos, y1 + 1, x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

  }

}


/*
void draw_scrollbar(struct sliderstruct* sl, int x_offset, int y_offset)
{

	if (sl->hidden == 1)
		return;

 int x1 = sl->x1 - x_offset;
 int y1 = sl->y1 - y_offset;
 int x2 = sl->x2 - x_offset;
 int y2 = sl->y2 - y_offset;

// first, draw background:
 al_draw_filled_rectangle(x1, y1, x2, y2, colours.base [sl->colour] [SHADE_MIN]);
// draw an outer border:
 al_draw_rectangle(x1, y1, x2, y2, colours.base [sl->colour] [SHADE_LOW], 1);

 if (sl->dir == SLIDEDIR_VERTICAL)
 {
// draw a button at each end:
  if (sl->highlighted == POINTER_BUTTON_LESS)
   al_draw_filled_rectangle(x1 + 1, y1 + 1, x2 - 1, y1 + SLIDER_BUTTON_SIZE, colours.base [sl->colour] [SHADE_LOW]);
  al_draw_rectangle(x1 + 1, y1 + 1, x2 - 1, y1 + SLIDER_BUTTON_SIZE, colours.base [sl->colour] [SHADE_MED], 1);
  al_draw_textf(font[FONT_BASIC].fnt, colours.base [sl->colour] [SHADE_MED], x1 + 9, y1 + 8, ALLEGRO_ALIGN_CENTRE, "^");
//  al_draw_line(x1 + 9, y1 + 7, x1 + 6, y1 + 12, colours.base [sl->colour] [SHADE_MED], 1);
//  al_draw_line(x1 + 9, y1 + 7, x1 + 12, y1 + 12, colours.base [sl->colour] [SHADE_MED], 1);
//  al_draw_triangle(x1 + 5, y1 + SLIDER_BUTTON_SIZE - 6, x1 + (SLIDER_BUTTON_SIZE/2) - 1, y1 + 5, x2 - 7, y1 + SLIDER_BUTTON_SIZE - 6, colours.base [sl->colour] [SHADE_MED], 1);
  if (sl->highlighted == POINTER_BUTTON_MORE)
   al_draw_filled_rectangle(x1 + 1, y2 - SLIDER_BUTTON_SIZE, x2 - 1, y2 - 1, colours.base [sl->colour] [SHADE_LOW]);
  al_draw_textf(font[FONT_BASIC].fnt, colours.base [sl->colour] [SHADE_MED], x1 + 9, y2 - SLIDER_BUTTON_SIZE + 1, ALLEGRO_ALIGN_CENTRE, "%s", down_arrow);
  al_draw_rectangle(x1 + 1, y2 - SLIDER_BUTTON_SIZE, x2 - 1, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

// draw slider:
   if (sl->highlighted == POINTER_SLIDER
    || sl->hold_type == SLIDER_HOLD_SLIDER)
   al_draw_filled_rectangle(x1 + 1, y1 + SLIDER_BUTTON_SIZE + sl->slider_pos, x2 - 1, y1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, colours.base [sl->colour] [SHADE_LOW]);
  al_draw_rectangle(x1 + 1, y1 + SLIDER_BUTTON_SIZE + sl->slider_pos, x2 - 1, y1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, colours.base [sl->colour] [SHADE_MED], 1);

 }
  else // SLIDEDIR_HORIZONTAL
  {
// draw a button at each end:
   if (sl->highlighted == POINTER_BUTTON_LESS)
    al_draw_filled_rectangle(x1 + 1, y1 + 1, x1 + SLIDER_BUTTON_SIZE, y2 - 1, colours.base [sl->colour] [SHADE_LOW]);
   al_draw_textf(font[FONT_BASIC].fnt, colours.base [sl->colour] [SHADE_MED], x1 + 9, y1 + 5, ALLEGRO_ALIGN_CENTRE, "<");
   al_draw_rectangle(x1 + 1, y1 + 1, x1 + SLIDER_BUTTON_SIZE, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

   if (sl->highlighted == POINTER_BUTTON_MORE)
    al_draw_filled_rectangle(x2 - SLIDER_BUTTON_SIZE, y1 + 1, x2 - 1, y2 - 1, colours.base [sl->colour] [SHADE_LOW]);
   al_draw_textf(font[FONT_BASIC].fnt, colours.base [sl->colour] [SHADE_MED], x2 - 9, y1 + 5, ALLEGRO_ALIGN_CENTRE, ">");
   al_draw_rectangle(x2 - SLIDER_BUTTON_SIZE, y1 + 1, x2 - 1, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

//   al_draw_rectangle(x1 + 1, y1 + 1, x1 + SLIDER_BUTTON_SIZE, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

// draw slider:
   if (sl->highlighted == POINTER_SLIDER
    || sl->hold_type == SLIDER_HOLD_SLIDER)
    al_draw_filled_rectangle(x1 + SLIDER_BUTTON_SIZE + sl->slider_pos, y1 + 1, x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, y2 - 1, colours.base [sl->colour] [SHADE_LOW]);
   al_draw_rectangle(x1 + SLIDER_BUTTON_SIZE + sl->slider_pos, y1 + 1, x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, y2 - 1, colours.base [sl->colour] [SHADE_MED], 1);

  }

}
*/

// x_offset and y_offset are needed to translate the absolute position of the scrollbar to the position on the current target bitmap
// both should be zero if drawing to display or similar
void draw_choosebar(struct sliderstruct* sl, int x_offset, int y_offset)
{

	if (sl->hidden == 1)
		return;

 int x1 = sl->x1 - x_offset;
 int y1 = sl->y1 - y_offset;
 int x2 = sl->x2 - x_offset;
 int y2 = sl->y2 - y_offset;

 int colour = sl->colour;
 int shade = SHADE_MED + 1;
 int shade2 = SHADE_LOW;
 int highlight_shade = SHADE_MED;
 int fill_shade = shade2;

// first, draw central line:
// al_draw_line(x1, y1 + (SLIDER_BUTTON_SIZE / 2), x2, y1 + (SLIDER_BUTTON_SIZE / 2), base_col [colour] [shade], 1);
 al_draw_line(x1 + SLIDER_BUTTON_SIZE, y1 + (SLIDER_BUTTON_SIZE / 2), x2 - SLIDER_BUTTON_SIZE, y1 + (SLIDER_BUTTON_SIZE / 2), colours.base [colour] [shade], 1);

// draw end lines:
// al_draw_line(x1, y1, x1, y2, base_col [COL_GREEN] [SHADE_MED]);
// al_draw_line(x2, y1, x2, y2, base_col [COL_GREEN] [SHADE_MED]);

// draw an outer border:
// al_draw_rectangle(x1, y1, x2, y2, base_col [COL_GREEN] [SHADE_LOW], 1);
/*
 if (sl->dir == SLIDEDIR_VERTICAL)
 {
// draw a button at each end:
  al_draw_rectangle(x1 + 1, y1 + 1, x2 - 1, y1 + SLIDER_BUTTON_SIZE, base_col [colour] [shade], 1);
  al_draw_rectangle(x1 + 1, y2 - SLIDER_BUTTON_SIZE, x2 - 1, y2 - 1, base_col [colour] [shade], 1);

// draw number at each end and in the middle:

// draw slider:
  al_draw_rectangle(x1 + 1 + (SLIDER_BUTTON_SIZE/4), y1 + SLIDER_BUTTON_SIZE + sl->slider_pos, x2 - 1 - (SLIDER_BUTTON_SIZE/4), y1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, base_col [colour] [shade], 1);


 }
  else*/
  {
// draw a button at left end:
   if (sl->highlighted == POINTER_BUTTON_LESS)
    fill_shade = highlight_shade;
     else
      fill_shade = shade2;
   al_draw_filled_rectangle(x1 + 1, y1 + 1, x1 + SLIDER_BUTTON_SIZE, y2 - 1, colours.base [colour] [fill_shade]);
   al_draw_rectangle(x1 + 1, y1 + 1, x1 + SLIDER_BUTTON_SIZE, y2 - 1, colours.base [colour] [shade], 1);
// left arrow
   al_draw_textf(font[FONT_BASIC].fnt, colours.base [colour] [shade], x1 + 13, y1 + 5, ALLEGRO_ALIGN_RIGHT, "<");

// right end
   if (sl->highlighted == POINTER_BUTTON_MORE)
    fill_shade = highlight_shade;
     else
      fill_shade = shade2;
   al_draw_filled_rectangle(x2 - SLIDER_BUTTON_SIZE, y1 + 1, x2 - 1, y2 - 1, colours.base [colour] [fill_shade]);
   al_draw_rectangle(x2 - SLIDER_BUTTON_SIZE, y1 + 1, x2 - 1, y2 - 1, colours.base [colour] [shade], 1);
// right arrow
   al_draw_textf(font[FONT_BASIC].fnt, colours.base [colour] [shade], x2 - 6, y1 + 5, ALLEGRO_ALIGN_RIGHT, ">");


// draw number at each end and in the middle:
   al_draw_textf(font[FONT_BASIC].fnt, colours.base [colour] [shade - 1], x1 - 1, y1 + 5, ALLEGRO_ALIGN_RIGHT, "%i", sl->value_min);
   al_draw_textf(font[FONT_BASIC].fnt, colours.base [colour] [shade - 1], x2 + 1, y1 + 5, ALLEGRO_ALIGN_LEFT, "%i", sl->value_max);
   al_draw_textf(font[FONT_BASIC].fnt, colours.base [colour] [shade], x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + (sl->slider_size / 2), y2 - 26, ALLEGRO_ALIGN_CENTRE, "%i", sl->value);
//  al_draw_textf(font, base_col [colour] [SHADE_MED], x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + (sl->slider_pos / 2), y2 + 3, ALLEGRO_ALIGN_CENTRE, "%i", sl->value);

// draw slider:
   if (sl->highlighted == POINTER_SLIDER
    || sl->hold_type == SLIDER_HOLD_SLIDER)
    fill_shade = highlight_shade;
     else
      fill_shade = shade2;
   al_draw_filled_rectangle(x1 + SLIDER_BUTTON_SIZE + sl->slider_pos, y1 + 1 + (SLIDER_BUTTON_SIZE/4), x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, y2 - 1 - (SLIDER_BUTTON_SIZE/4), colours.base [colour] [fill_shade]);
   al_draw_rectangle(x1 + SLIDER_BUTTON_SIZE + sl->slider_pos, y1 + 1 + (SLIDER_BUTTON_SIZE/4), x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size, y2 - 1 - (SLIDER_BUTTON_SIZE/4), colours.base [colour] [shade], 1);

/*
   float slx1 = x1 + SLIDER_BUTTON_SIZE + sl->slider_pos - 0.5;
   float slx2 = x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size + 0.5;
   float sly1 = y1 + (SLIDER_BUTTON_SIZE / 2) - 2 - 0.5;
   float sly2 = y1 + (SLIDER_BUTTON_SIZE / 2) + 2 + 0.5;

   al_draw_line(slx1, sly1, slx2, sly1, base_col [colour] [shade], 1);
   al_draw_line(slx1, sly1, slx1, sly1 - 2, base_col [colour] [shade], 1);
   al_draw_line(slx2, sly1, slx2, sly1 - 2, base_col [colour] [shade], 1);
   al_draw_line(slx1, sly2, slx2, sly2, base_col [colour] [shade], 1);
   al_draw_line(slx1, sly2, slx1, sly2 + 2, base_col [colour] [shade], 1);
   al_draw_line(slx2, sly2, slx2, sly2 + 2, base_col [colour] [shade], 1);*/


  }

}



#define BASE_HOLD_DELAY 5
#define SHORT_HOLD_DELAY 2

// use x_offset and y_offset when the slider has changed position (e.g. because it is in a menu element and the menu has scrolled up or down)
void run_slider(struct sliderstruct* sl, int x_offset, int y_offset)
{

  int pointer_pos = POINTER_OUTSIDE;
  sl->highlighted = POINTER_OUTSIDE;
  sl->hidden = sl->hidden_if_unused; // if 1, may be set to zero below

  int slider_x1 = sl->x1 - x_offset;
  int slider_x2 = sl->x2 - x_offset;
  int slider_y1 = sl->y1 - y_offset;
  int slider_y2 = sl->y2 - y_offset;

  if (ex_control.mouse_x_pixels >= slider_x1
   && ex_control.mouse_x_pixels <= slider_x2
   && ex_control.mouse_y_pixels >= slider_y1
   && ex_control.mouse_y_pixels <= slider_y2)
  {
     if (sl->dir == SLIDEDIR_VERTICAL)
  {
   if (ex_control.mouse_y_pixels <= slider_y1 + SLIDER_BUTTON_SIZE)
    pointer_pos = POINTER_BUTTON_LESS;
     else
     {
      if (ex_control.mouse_y_pixels >= slider_y2 - SLIDER_BUTTON_SIZE)
       pointer_pos = POINTER_BUTTON_MORE;
        else
        {
         if (ex_control.mouse_y_pixels < slider_y1 + SLIDER_BUTTON_SIZE + sl->slider_pos)
          pointer_pos = POINTER_TRACK_LESS;
           else
           {
            if (ex_control.mouse_y_pixels> slider_y1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size)
             pointer_pos = POINTER_TRACK_MORE;
              else
              {
               pointer_pos = POINTER_SLIDER;
              }
           }
        }
     }
  }  // end if vertical slider
   else
  {
   if (ex_control.mouse_x_pixels <= slider_x1 + SLIDER_BUTTON_SIZE)
    pointer_pos = POINTER_BUTTON_LESS;
     else
     {
      if (ex_control.mouse_x_pixels >= slider_x2 - SLIDER_BUTTON_SIZE)
       pointer_pos = POINTER_BUTTON_MORE;
        else
        {
         if (ex_control.mouse_x_pixels < slider_x1 + SLIDER_BUTTON_SIZE + sl->slider_pos)
          pointer_pos = POINTER_TRACK_LESS;
           else
           {
            if (ex_control.mouse_x_pixels > slider_x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size)
             pointer_pos = POINTER_TRACK_MORE;
              else
              {
               pointer_pos = POINTER_SLIDER;
              }
           }
        }
     }
  } // end else for horizontal slider
  sl->highlighted = pointer_pos;
  sl->hidden = 0;
  }


// don't otherwise interact with slider if button not pressed
 if (ex_control.mb_press [0] <= 0)
 {
  sl->hold_delay = 0;
  sl->hold_type = SLIDER_HOLD_NONE;
  return;
 }

// don't start interacting with slider if button is being held after being first pressed elsewhere:
 if (ex_control.mb_press [0] == BUTTON_HELD
  && sl->hold_type == SLIDER_HOLD_NONE)
 {
  sl->hold_delay = 0;
  return;
 }

 if (sl->hold_delay > 0)
  sl->hold_delay --;

 if (ex_control.using_slider
  && sl->hold_type == SLIDER_HOLD_NONE)
 {
// a slider is being used, but not this one.
  return;
 }

 if (sl->hold_type != SLIDER_HOLD_SLIDER) // this is handled differently from the other types of hold (see below)
 {
// first we check whether the pointer is within the slider at all
  if (ex_control.mouse_x_pixels < slider_x1
   || ex_control.mouse_x_pixels > slider_x2
   || ex_control.mouse_y_pixels < slider_y1
   || ex_control.mouse_y_pixels > slider_y2)
    return;
// since we've already checked for the mouse being clicked, and we know that the mouse is within this slider, and we know that no other slider is being used, we set using_slider:
  ex_control.using_slider = 1;
// now we work out where the pointer is within the slider.
// vertical and horizontal are handled separately here:
/*  if (sl->dir == SLIDEDIR_VERTICAL)
  {
   if (ex_control.mouse_y_pixels <= slider_y1 + SLIDER_BUTTON_SIZE)
    pointer_pos = POINTER_BUTTON_LESS;
     else
     {
      if (ex_control.mouse_y_pixels >= slider_y2 - SLIDER_BUTTON_SIZE)
       pointer_pos = POINTER_BUTTON_MORE;
        else
        {
         if (ex_control.mouse_y_pixels < slider_y1 + SLIDER_BUTTON_SIZE + sl->slider_pos)
          pointer_pos = POINTER_TRACK_LESS;
           else
           {
            if (ex_control.mouse_y_pixels> slider_y1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size)
             pointer_pos = POINTER_TRACK_MORE;
              else
              {
               pointer_pos = POINTER_SLIDER;
              }
           }
        }
     }
  }  // end if vertical slider
   else
  {
   if (ex_control.mouse_x_pixels <= slider_x1 + SLIDER_BUTTON_SIZE)
    pointer_pos = POINTER_BUTTON_LESS;
     else
     {
      if (ex_control.mouse_x_pixels >= slider_x2 - SLIDER_BUTTON_SIZE)
       pointer_pos = POINTER_BUTTON_MORE;
        else
        {
         if (ex_control.mouse_x_pixels < slider_x1 + SLIDER_BUTTON_SIZE + sl->slider_pos)
          pointer_pos = POINTER_TRACK_LESS;
           else
           {
            if (ex_control.mouse_x_pixels > slider_x1 + SLIDER_BUTTON_SIZE + sl->slider_pos + sl->slider_size)
             pointer_pos = POINTER_TRACK_MORE;
              else
              {
               pointer_pos = POINTER_SLIDER;
              }
           }
        }
     }
  } // end else for horizontal slider
*/
// now we know which part of the slider the pointer is in:

   switch(sl->hold_type)
   {
    case SLIDER_HOLD_NONE:
// check for clicks in slider area, which will indicate that the player has clicked on it
     switch(pointer_pos)
     {
      case POINTER_BUTTON_LESS:
       slider_button_or_track_clicked(sl, 0 - sl->button_increment);
       sl->hold_type = SLIDER_HOLD_BUTTON;
       sl->hold_delay = BASE_HOLD_DELAY;
       return;
      case POINTER_BUTTON_MORE:
       slider_button_or_track_clicked(sl, sl->button_increment);
       sl->hold_type = SLIDER_HOLD_BUTTON;
       sl->hold_delay = BASE_HOLD_DELAY;
       return;
      case POINTER_TRACK_LESS:
       slider_button_or_track_clicked(sl, 0 - sl->track_increment);
       sl->hold_type = SLIDER_HOLD_TRACK;
       sl->hold_delay = BASE_HOLD_DELAY;
       return;
      case POINTER_TRACK_MORE:
       slider_button_or_track_clicked(sl, sl->track_increment);
       sl->hold_type = SLIDER_HOLD_TRACK;
       sl->hold_delay = BASE_HOLD_DELAY;
       return;
      case POINTER_SLIDER:
       sl->hold_type = SLIDER_HOLD_SLIDER;
       if (sl->dir == SLIDEDIR_VERTICAL)
        sl->hold_offset = ex_control.mouse_y_pixels - slider_y1 - SLIDER_BUTTON_SIZE - sl->slider_pos;
         else
          sl->hold_offset = ex_control.mouse_x_pixels - slider_x1 - SLIDER_BUTTON_SIZE - sl->slider_pos;
       sl->hold_base_value = sl->value;
//       sl->hold_delay = BASE_HOLD_DELAY; no hold delay
       return;
     } // end switch(pointer_pos)
     break; // end SLIDER_HOLD_NONE
    case SLIDER_HOLD_BUTTON:
     if (pointer_pos == POINTER_BUTTON_LESS)
     {
      if (sl->hold_delay == 0)
      {
       slider_button_or_track_clicked(sl, 0 - sl->button_increment);
       sl->hold_delay = SHORT_HOLD_DELAY;
      }
      return;
     }
     if (pointer_pos == POINTER_BUTTON_MORE)
     {
      if (sl->hold_delay == 0)
      {
       slider_button_or_track_clicked(sl, sl->button_increment);
       sl->hold_delay = SHORT_HOLD_DELAY;
      }
      return;
     }
     break; // end case SLIDER_HOLD_BUTTON
    case SLIDER_HOLD_TRACK:
     if (pointer_pos == POINTER_TRACK_LESS)
     {
      if (sl->hold_delay == 0)
      {
       slider_button_or_track_clicked(sl, 0 - sl->track_increment);
       sl->hold_delay = SHORT_HOLD_DELAY;
      }
      return;
     }
     if (pointer_pos == POINTER_TRACK_MORE)
     {
      if (sl->hold_delay == 0)
      {
       slider_button_or_track_clicked(sl, sl->track_increment);
       sl->hold_delay = SHORT_HOLD_DELAY;
      }
      return;
     }
     return;
   } // end switch(hold_type)

  } // end if (hold_type != slider)
   else // now deal with the user grabbing the slider with the mouse button and moving it
   {
   	sl->hidden = 0; // don't hide the slider in this mode
    if (sl->dir == SLIDEDIR_VERTICAL)
    {
     if (ex_control.mouse_x_pixels < slider_x1 - 80
      || ex_control.mouse_x_pixels > slider_x1 + 80 + SLIDER_BUTTON_SIZE)
     {
//      slider_moved(sl, sl->hold_base_value * sl->value_per_pixel);
      slider_moved_to_value(sl, sl->hold_base_value);
     }
      else
       slider_moved_to_pos(sl, ex_control.mouse_y_pixels - (slider_y1 + SLIDER_BUTTON_SIZE) - sl->hold_offset);
    }
     else // must be horizontal
      {
       if (ex_control.mouse_y_pixels < slider_y1 - 80
        || ex_control.mouse_y_pixels > slider_y1 + 80 + SLIDER_BUTTON_SIZE)
       {
//        slider_moved(sl, sl->hold_base_value * sl->value_per_pixel);
        slider_moved_to_value(sl, sl->hold_base_value);
       }
        else
         slider_moved_to_pos(sl, ex_control.mouse_x_pixels - (slider_x1 + SLIDER_BUTTON_SIZE) - sl->hold_offset);
      }
   } // end hold_type == slider else code

// may have returned before here

} // end run_slider()


