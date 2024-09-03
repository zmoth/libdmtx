/**
 * libdmtx - Data Matrix Encoding/Decoding Library
 * Copyright 2007, 2008, 2009 Mike Laughton. All rights reserved.
 *
 * See LICENSE file in the main project directory for full
 * terms of use and distribution.
 *
 * Contact: Mike Laughton <mike@dragonflylogic.com>
 *
 * \file display.c
 */

#include "display.h"

#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "rotate_test.h"

SDL_Window *window;
SDL_Surface *screen;

/**
 *
 *
 */
SDL_Surface *initDisplay(void)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Video initialization failed: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow("GL Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 968, 646,
                              SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        exit(2);
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    assert(context);

    screen = SDL_GetWindowSurface(window);
    if (!screen) {
        fprintf(stderr, "Couldn't set 968x646 GL video mode: %s\n", SDL_GetError());
        exit(1);
    }

    glClearColor(0.0F, 0.0F, 0.3F, 1.0F);

    return screen;
}

/**
 *
 *
 */
void DrawBarCode(void)
{
    glColor3f(0.95F, 0.95F, 0.95F);
    glBegin(GL_QUADS);
    glTexCoord2d(0.0, 0.0);
    glVertex3f(-2.0F, -2.0F, 0.0F);
    glTexCoord2d(1.0, 0.0);
    glVertex3f(2.0F, -2.0F, 0.0F);
    glTexCoord2d(1.0, 1.0);
    glVertex3f(2.0F, 2.0F, 0.0F);
    glTexCoord2d(0.0, 1.0);
    glVertex3f(-2.0F, 2.0F, 0.0F);
    glEnd();
}

/**
 *
 *
 */
void ReshapeWindow(int width, int height)
{
    glViewport(2, 324, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 50.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

/**
 *
 *
 */
void DrawBorders(SDL_Surface *screen)
{
    /* window and pane borders */
    DrawPaneBorder(0, 0, 646, 968);

    DrawPaneBorder(1, 1, 322, 322);
    DrawPaneBorder(323, 1, 322, 322);
    DrawPaneBorder(645, 1, 322, 322);

    DrawPaneBorder(1, 323, 322, 322);
    DrawPaneBorder(323, 323, 322, 322);
    DrawPaneBorder(645, 323, 322, 322);
}

/**
 *
 *
 */
void DrawGeneratedImage(SDL_Surface *screen)
{
    /* rotate barcode surface */
    glViewport(2, 324, texturePxl->w, texturePxl->h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 50.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0F, 0.0F, -10.0F);
    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_LINE);
    glEnable(GL_TEXTURE_2D);

    glPushMatrix();
    glRotatef(view_rotx, 1.0F, 0.0F, 0.0F);
    glRotatef(view_roty, 0.0F, 1.0F, 0.0F);
    glRotatef(view_rotz, 0.0F, 0.0F, 1.0F);
    glRotatef(angle, 0.0F, 0.0F, 1.0F);
    glCallList(barcodeList);
    glPopMatrix();
}

/**
 *
 *
 */
void DrawPane2(SDL_Surface *screen, unsigned char *pxl)
{
    DrawPaneBorder(323, 323, 322, 322); /* XXX drawn twice */
    glRasterPos2i(1, 1);
    glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
}

/**
 *
 *
 */
void DrawPane3(SDL_Surface *screen, unsigned char *pxl)
{
    DrawPaneBorder(645, 323, 322, 322); /* XXX drawn twice */
    glRasterPos2i(1, 1);
    glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
}

/**
 *
 *
 */
void DrawPane4(SDL_Surface *screen, unsigned char *pxl)
{
    DrawPaneBorder(1, 1, 322, 322); /* XXX drawn twice */
    glRasterPos2i(1, 1);
    glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
}

/**
 *
 *
 */
void DrawPane5(SDL_Surface *screen, unsigned char *pxl)
{
    DrawPaneBorder(323, 1, 322, 322); /* XXX drawn twice */
    glRasterPos2i(1, 1);
    glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
}

/**
 *
 *
 */
void DrawPane6(SDL_Surface *screen, unsigned char *pxl)
{
    DrawPaneBorder(645, 1, 322, 322); /* XXX drawn twice */
    glRasterPos2i(1, 1);

    if (pxl != NULL) {
        glDrawPixels(320, 320, GL_RGB, GL_UNSIGNED_BYTE, pxl);
    }
}

/**
 *
 *
 */
void DrawPaneBorder(GLint x, GLint y, GLint h, GLint w)
{
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.6F, 0.6F, 1.0F);
    glPolygonMode(GL_FRONT, GL_LINE);
    glViewport(x, y, w, w);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-0.5, w - 0.5, -0.5, w - 0.5, -1.0, 10.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f((GLfloat)(w - 1), 0);
    glVertex2f((GLfloat)(w - 1), (GLfloat)(h - 1));
    glVertex2f(0, (GLfloat)(h - 1));
    glEnd();
}

/**
 *
 *
 */
int HandleEvent(SDL_Event *event, SDL_Surface *screen)
{
    switch (event->type) {
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_RESIZED) {  // 获取新的窗口尺寸
                int width = event->window.data1;
                int height = event->window.data2;
                ReshapeWindow(width, height);
            }
            break;

        case SDL_QUIT:
            return (1);
            break;

        case SDL_MOUSEMOTION:
            view_rotx = ((float)event->motion.y - 160.0F) / 2.0F;
            view_roty = ((float)event->motion.x - 160.0F) / 2.0F;
            break;

        case SDL_KEYDOWN:
            switch (event->key.keysym.sym) {
                case SDLK_ESCAPE:
                    return (1);
                    break;
                default:
                    break;
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            switch (event->button.button) {
                case SDL_BUTTON_RIGHT:
                    // 下一张测试图片
                    SDL_FreeSurface(texturePxl);
                    texturePxl = loadTextureImage();
                    break;
                case SDL_BUTTON_LEFT:
                    fprintf(stdout, "left click\n");
                    break;
                default:
                    break;
            }
            break;
    }

    return (0);
}
