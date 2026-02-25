/*
 * Copyright (c) 1993-1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED
 * Permission to use, copy, modify, and distribute this software for
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

/*
 *  light.c
 *  This program demonstrates the use of the OpenGL lighting
 *  model.  A sphere is drawn using a grey material characteristic.
 *  A single light source illuminates the object.
 */
#include <GL/glut.h>
#include <stdlib.h>

/*  Initialize material property, light source, lighting model,
 *  and depth buffer.
 */

void addLight1(void)
{
   // declara a cor (R,G,B,A) e posicao (x,y,z,w) da fonte de luz
   // GLfloat light1_ambient[] = {0.2, 0.2, 0.2, 1.0};
   GLfloat light1_diffuse[] = {0.0, 1.0, 0.0, 1.0};
   GLfloat light1_specular[] = {0.0, 1.0, 0.0, 1.0};
   GLfloat light1_position[] = {-2.0, 1.0, 1.0, 1.0};
   GLfloat spot_direction[] = {2.0, 0.0, -1.0};
   // atribui os valores de reflexao e posicao
   // glLightfv(GL_LIGHT1, GL_AMBIENT, light1_ambient);
   glLightfv(GL_LIGHT1, GL_DIFFUSE, light1_diffuse);
   glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);
   glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
   // atribui as atenuacoes
   glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 1.5);
   glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.5);
   glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 0.2);
   // atribui as caracteristicas da luz tipo lanterna
   glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 45.0);
   glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, spot_direction);
   glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 2.0);
   // habilita a luz 1
   glEnable(GL_LIGHT1);
}

void init(void)
{
   // Material branco/cinza para refletir todas as cores
   GLfloat mat_specular[] = {1.0, 1.0, 1.0, 1.0};
   GLfloat mat_diffuse[] = {0.8, 0.8, 0.8, 1.0};
   GLfloat mat_shininess[] = {100.0};
   
   // LIGHT0 - Luz vermelha da direita
   GLfloat light0_diffuse[] = {1.0, 0.0, 0.0, 1.0};
   GLfloat light0_specular[] = {1.0, 0.0, 0.0, 1.0};
   GLfloat light_position[] = {1.0, 1.0, 1.0, 1.0};

   glClearColor(0.0, 0.0, 0.0, 0.0);
   glShadeModel(GL_SMOOTH);

   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);   
   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);
   
   glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
   glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
   glLightfv(GL_LIGHT0, GL_POSITION, light_position);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

      // declara a cor (R,G,B,A) e posicao (x,y,z,w) da fonte de luz

   
   addLight1();

}

void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glutSolidSphere(1.0, 20, 16);
   glFlush();
}

void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei)w, (GLsizei)h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   if (w <= h)
      glOrtho(-1.5, 1.5, -1.5 * (GLfloat)h / (GLfloat)w,
              1.5 * (GLfloat)h / (GLfloat)w, -10.0, 10.0);
   else
      glOrtho(-1.5 * (GLfloat)w / (GLfloat)h,
              1.5 * (GLfloat)w / (GLfloat)h, -1.5, 1.5, -10.0, 10.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

void keyboard(unsigned char key, int x, int y)
{
   switch (key)
   {
   case 27:
      exit(0);
      break;
   }
}

int main(int argc, char **argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize(500, 500);
   glutInitWindowPosition(100, 100);
   glutCreateWindow(argv[0]);
   init();
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutMainLoop();
   return 0;
}
