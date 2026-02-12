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
 * robot.c
 * This program shows how to composite modeling transformations
 * to draw translated and rotated hierarchical models.
 * Interaction:  pressing the b, s, e, t, w, f, g keys
 * alters the rotation of the robot arm.
 * 
 * Controls:
 * b/B - Rotate base (horizontal rotation)
 * s/S - Rotate shoulder (up arm)
 * e/E - Rotate elbow
 * t/T - Twist forearm (torção do antebraço)
 * w/W - Rotate wrist
 * f/F - Open/close end effector fingers
 * g/G - Grab/release sphere (pega/solta a esfera)
 * ESC - Exit
 */
#include <GL/glut.h>
#include <stdlib.h>

// Ângulos de rotação para cada junta do robô
static int base = 0;       // Rotação da base (horizontal)
static int shoulder = 0;   // Junta do ombro (braço superior)
static int elbow = 0;      // Junta do cotovelo
static int twist = 0;      // Torção do antebraço (rotação no próprio eixo)
static int wrist = 0;      // Junta do pulso
static int fingers = 0;    // Abertura dos dedos do end effector

// Estado de controle da esfera
static int grabbed = 0;    // 0 = esfera livre, 1 = esfera na mão do robô

void init(void)
{
   glEnable(GL_DEPTH_TEST);
   glClearColor(0.0, 0.0, 0.0, 0.0);
   glShadeModel(GL_FLAT);
}

void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   
   glPushMatrix();
   
   // BASE RETANGULAR (chão) - rotaciona horizontalmente
   glRotatef((GLfloat)base, 0.0, 1.0, 0.0);  // Rotação horizontal (eixo Y)
   
   glPushMatrix();
   glTranslatef(0.0, -1.5, 0.0);  // Posiciona a base no chão
   glScalef(2.0, 0.3, 1.5);       // Base retangular larga
   glColor3f(0.3, 0.3, 0.3);      // Cinza escuro
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);            // Wireframe branco
   glutWireCube(1.001);
   glPopMatrix();
   
   // SHOULDER (braço superior) - rotaciona verticalmente
   glTranslatef(0.0, -1.35, 0.0); // Posiciona acima da base
   glRotatef((GLfloat)shoulder, 0.0, 0.0, 1.0);  // Rotação do ombro (eixo Z)
   glTranslatef(0.0, 1.0, 0.0);   // Desloca para desenhar o braço
   
   glPushMatrix();
   glScalef(0.4, 2.0, 0.4);       // Braço vertical e fino
   glColor3f(1.0, 0.0, 0.0);      // Vermelho
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // ELBOW (antebraço) - rotaciona no final do shoulder
   glTranslatef(0.0, 1.0, 0.0);   // Move para o topo do braço superior
   glRotatef((GLfloat)elbow, 0.0, 0.0, 1.0);     // Rotação do cotovelo (eixo Z)
   glTranslatef(0.0, 1.0, 0.0);   // Desloca para desenhar o antebraço
   
   // TWIST (torção do antebraço) - rotaciona no próprio eixo
   glRotatef((GLfloat)twist, 0.0, 1.0, 0.0);     // Rotação de torção (eixo Y)
   
   glPushMatrix();
   glScalef(0.35, 2.0, 0.35);     // Antebraço um pouco mais fino
   glColor3f(0.0, 1.0, 0.0);      // Verde
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // WRIST (pulso) - rotaciona no final do elbow
   glTranslatef(0.0, 1.0, 0.0);   // Move para o topo do antebraço
   glRotatef((GLfloat)wrist, 0.0, 0.0, 1.0);     // Rotação do pulso (eixo Z)
   glTranslatef(0.0, 0.4, 0.0);   // Desloca para desenhar o pulso
   
   glPushMatrix();
   glScalef(0.3, 0.8, 0.3);       // Pulso pequeno
   glColor3f(0.0, 0.0, 1.0);      // Azul
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // END EFFECTOR (garra com 2 dedos, cada um com 2 segmentos)
   glTranslatef(0.0, 0.4, 0.0);   // Move para o topo do pulso
   
   // DEDO 1 (esquerdo)
   glPushMatrix();
   glRotatef((GLfloat)fingers, 0.0, 0.0, 1.0);   // Abre/fecha rotacionando em Z
   
   // Primeiro segmento do dedo 1
   glTranslatef(-0.15, 0.25, 0.0);
   glPushMatrix();
   glRotatef(-20.0, 0.0, 0.0, 1.0);  // Inclina levemente
   glScalef(0.12, 0.5, 0.12);
   glColor3f(1.0, 1.0, 0.0);      // Amarelo
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // Segundo segmento do dedo 1
   glTranslatef(0.0, 0.25, 0.0);
   glRotatef(-15.0, 0.0, 0.0, 1.0);  // Inclina mais
   glTranslatef(0.0, 0.2, 0.0);
   glScalef(0.1, 0.4, 0.1);
   glColor3f(1.0, 0.8, 0.0);      // Amarelo mais escuro
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // DEDO 2 (direito)
   glPushMatrix();
   glRotatef(-(GLfloat)fingers, 0.0, 0.0, 1.0);  // Abre/fecha no sentido oposto
   
   // Primeiro segmento do dedo 2
   glTranslatef(0.15, 0.25, 0.0);
   glPushMatrix();
   glRotatef(20.0, 0.0, 0.0, 1.0);   // Inclina levemente (oposto)
   glScalef(0.12, 0.5, 0.12);
   glColor3f(1.0, 1.0, 0.0);      // Amarelo
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // Segundo segmento do dedo 2
   glTranslatef(0.0, 0.25, 0.0);
   glRotatef(15.0, 0.0, 0.0, 1.0);   // Inclina mais (oposto)
   glTranslatef(0.0, 0.2, 0.0);
   glScalef(0.1, 0.4, 0.1);
   glColor3f(1.0, 0.8, 0.0);      // Amarelo mais escuro
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // ESFERA (desenhada aqui se estiver na mão - grabbed == 1)
   // A esfera segue a transformação hierárquica da mão
   if (grabbed == 1) {
      glPushMatrix();
      glTranslatef(0.0, 0.5, 0.0);    // Posiciona entre os dedos
      glColor3f(0.8, 0.2, 0.2);       // Vermelho escuro
      glutSolidSphere(0.5, 20, 20);   // Esfera com raio 0.5
      glColor3f(1, 1, 1);             // Wireframe branco
      glutWireSphere(0.501, 12, 12);
      glPopMatrix();
   }
   
   glPopMatrix();
   
   glPopMatrix();
   
   // ESFERA (no chão, só aparece se grabbed == 0)
   if (grabbed == 0) {
      glPushMatrix();
      glTranslatef(3.5, -0.8, 0.0);   // Posiciona ao lado direito do robô
      glColor3f(0.8, 0.2, 0.2);       // Vermelho escuro
      glutSolidSphere(0.5, 20, 20);   // Esfera com raio 0.5
      glColor3f(1, 1, 1);             // Wireframe branco
      glutWireSphere(0.501, 12, 12);
      glPopMatrix();
   }
   
   // CAIXA COM TAMPA ABERTA (ao lado do robô)
   glPushMatrix();
   glTranslatef(-3.0, -1.2, 0.0);  // Posiciona ao lado esquerdo do robô
   
   // Base da caixa
   glPushMatrix();
   glScalef(1.2, 0.8, 1.2);
   glColor3f(0.6, 0.4, 0.2);       // Marrom
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // Parede frontal
   glPushMatrix();
   glTranslatef(0.0, 0.4, 0.6);
   glScalef(1.2, 0.8, 0.05);
   glColor3f(0.6, 0.4, 0.2);
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // Parede traseira
   glPushMatrix();
   glTranslatef(0.0, 0.4, -0.6);
   glScalef(1.2, 0.8, 0.05);
   glColor3f(0.6, 0.4, 0.2);
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // Parede esquerda
   glPushMatrix();
   glTranslatef(-0.6, 0.4, 0.0);
   glScalef(0.05, 0.8, 1.2);
   glColor3f(0.6, 0.4, 0.2);
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // Parede direita
   glPushMatrix();
   glTranslatef(0.6, 0.4, 0.0);
   glScalef(0.05, 0.8, 1.2);
   glColor3f(0.6, 0.4, 0.2);
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   // Tampa (aberta, rotacionada 110 graus para trás)
   glPushMatrix();
   glTranslatef(0.0, 0.8, -0.6);   // Posiciona na borda traseira
   glRotatef(-110.0, 1.0, 0.0, 0.0); // Rotaciona para trás (tampa aberta)
   glTranslatef(0.0, 0.0, 0.6);    // Ajusta pivô
   glScalef(1.2, 0.05, 1.2);
   glColor3f(0.5, 0.35, 0.15);     // Marrom mais escuro
   glutSolidCube(1.0);
   glColor3f(1, 1, 1);
   glutWireCube(1.001);
   glPopMatrix();
   
   glPopMatrix();
   
   glutSwapBuffers();
}

void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei)w, (GLsizei)h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(65.0, (GLfloat)w / (GLfloat)h, 1.0, 30.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   
   // Posiciona a câmera para ver o robô inteiro e os objetos ao lado
   gluLookAt(4.0, 2.0, 8.0,   // Posição da câmera (mais afastada e mais alta)
             0.0, 0.5, 0.0,    // Olha para o centro da cena
             0.0, 1.0, 0.0);   // Vetor "up"
}

void keyboard(unsigned char key, int x, int y)
{
   switch (key)
   {
   case 'b':  // Rotaciona base (horizontal) no sentido anti-horário
      base = (base + 5) % 360;
      glutPostRedisplay();
      break;
   case 'B':  // Rotaciona base (horizontal) no sentido horário
      base = (base - 5) % 360;
      glutPostRedisplay();
      break;
   case 's':  // Rotaciona shoulder (braço superior) no sentido anti-horário
      shoulder = (shoulder + 5) % 360;
      glutPostRedisplay();
      break;
   case 'S':  // Rotaciona shoulder (braço superior) no sentido horário
      shoulder = (shoulder - 5) % 360;
      glutPostRedisplay();
      break;
   case 'e':  // Rotaciona elbow (cotovelo) no sentido anti-horário
      elbow = (elbow + 5) % 360;
      glutPostRedisplay();
      break;
   case 'E':  // Rotaciona elbow (cotovelo) no sentido horário
      elbow = (elbow - 5) % 360;
      glutPostRedisplay();
      break;
   case 't':  // Torce antebraço (twist) no sentido anti-horário
      twist = (twist + 5) % 360;
      glutPostRedisplay();
      break;
   case 'T':  // Torce antebraço (twist) no sentido horário
      twist = (twist - 5) % 360;
      glutPostRedisplay();
      break;
   case 'w':  // Rotaciona wrist (pulso) no sentido anti-horário
      wrist = (wrist + 5) % 360;
      glutPostRedisplay();
      break;
   case 'W':  // Rotaciona wrist (pulso) no sentido horário
      wrist = (wrist - 5) % 360;
      glutPostRedisplay();
      break;
   case 'f':  // Abre os dedos do end effector
      if (fingers < 30) fingers += 5;  // Limita abertura máxima
      glutPostRedisplay();
      break;
   case 'F':  // Fecha os dedos do end effector
      if (fingers > 0) fingers -= 5;   // Limita fechamento mínimo
      glutPostRedisplay();
      break;
   case 'g':  // Pega a esfera (grab)
   case 'G':  // Solta a esfera (release)
      grabbed = !grabbed;  // Alterna entre pegar (1) e soltar (0)
      glutPostRedisplay();
      break;
   case 27:   // ESC - sai do programa
      exit(0);
      break;
   default:
      break;
   }
}

int main(int argc, char **argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
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
