#ifndef RAY_H
#define RAY_H

#include <GLFW/glfw3.h>
#include "Math.h"
#include "Wormhole.h"

struct Ray {
    double x, y;
    double r, phi;
    double dr, dphi;
    vector<vec2> trail;
    double E, L;
    
    // Store which mouth the ray is currently around (0 for A, 1 for B)
    int currentMouth; 
    Wormhole* wormhole;

    Ray(vec2 pos, vec2 dir, Wormhole* wh) : x(pos.x), y(pos.y), currentMouth(0), wormhole(wh) {
        initAroundMouth(wh->mouthA.position.x, wh->mouthA.position.y, dir);
    }
    
    void initAroundMouth(double mx, double my, vec2 dir) {
        double lx = x - mx;
        double ly = y - my;
        
        r = sqrt(lx*lx + ly*ly);
        phi = atan2(ly, lx);
        
        dr = dir.x * cos(phi) + dir.y * sin(phi);
        dphi = (-dir.x * sin(phi) + dir.y * cos(phi)) / r;
        
        L = r * r * dphi;
        double rs = currentMouth == 0 ? wormhole->mouthA.r_s : wormhole->mouthB.r_s;
        double f = 1.0 - rs / r;
        if(f <= 0.0) f = 0.0001; // Avoid division by zero
        
        double dt_dlam = sqrt((dr*dr)/(f*f) + (r*r*dphi*dphi)/f);
        E = f * dt_dlam;
        
        trail.push_back({x, y});
    }

    void draw(const std::vector<Ray>& rays) {
        glPointSize(2.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_POINTS);
        for (const auto& ray : rays) {
            glVertex2f(ray.x, ray.y);
        }
        glEnd();
    
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(1.0f);
    
        for (const auto& ray : rays) {
            size_t N = ray.trail.size();
            if (N < 2) continue;
    
            glBegin(GL_LINE_STRIP);
            for (size_t i = 1; i < N; ++i) {
                // To avoid drawing the huge line connecting mouths
                double dx = ray.trail[i].x - ray.trail[i-1].x;
                double dy = ray.trail[i].y - ray.trail[i-1].y;
                if (sqrt(dx*dx + dy*dy) > 1e10) {
                    glEnd();
                    glBegin(GL_LINE_STRIP);
                    continue; // Skip the jump
                }
                float alpha = float(i) / float(N - 1);
                glColor4f(1.0f, 1.0f, 1.0f, max(alpha, 0.05f));
                glVertex2f(ray.trail[i].x, ray.trail[i].y);
            }
            glEnd();
        }
    
        glDisable(GL_BLEND);
    }

    void updateClosestMouth() {
        double distA = sqrt(pow(x - wormhole->mouthA.position.x, 2) + pow(y - wormhole->mouthA.position.y, 2));
        double distB = sqrt(pow(x - wormhole->mouthB.position.x, 2) + pow(y - wormhole->mouthB.position.y, 2));
        
        int closestMouth = (distA < distB) ? 0 : 1;
        
        if (closestMouth != currentMouth) {
            // First calculate cartesian velocities PRECISELY from current mouth
            double vx = dr * cos(phi) - r * dphi * sin(phi);
            double vy = dr * sin(phi) + r * dphi * cos(phi);
            
            currentMouth = closestMouth;
            double mx = currentMouth == 0 ? wormhole->mouthA.position.x : wormhole->mouthB.position.x;
            double my = currentMouth == 0 ? wormhole->mouthA.position.y : wormhole->mouthB.position.y;
            
            double lx = x - mx;
            double ly = y - my;
            
            r = sqrt(lx*lx + ly*ly);
            phi = atan2(ly, lx);
            
            // Recompute dr and dphi from cartesian velocities
            dr = vx * cos(phi) + vy * sin(phi);
            dphi = (-vx * sin(phi) + vy * cos(phi)) / r;
            
            // Re-evaluate E and L
            double rs = currentMouth == 0 ? wormhole->mouthA.r_s : wormhole->mouthB.r_s;
            double f = 1.0 - rs / r;
            if(f <= 0.0) f = 0.0001;
            
            double dt_dlam_new = sqrt((dr*dr)/(f*f) + (r*r*dphi*dphi)/f);
            E = f * dt_dlam_new;
            L = r * r * dphi;
        }
    }

    void step(double dlam) {
        updateClosestMouth();
        
        double rs = currentMouth == 0 ? wormhole->mouthA.r_s : wormhole->mouthB.r_s;
        double mx = currentMouth == 0 ? wormhole->mouthA.position.x : wormhole->mouthB.position.x;
        double my = currentMouth == 0 ? wormhole->mouthA.position.y : wormhole->mouthB.position.y;

        if (r <= rs * 1.01) { 
            // Teleport to the other mouth!
            currentMouth = 1 - currentMouth; // Toggle 0 and 1
            double target_rs = currentMouth == 0 ? wormhole->mouthA.r_s : wormhole->mouthB.r_s;
            
            // Keep the same radial and angular momentum but move to the other mouth
            // Add a slight radial outward velocity
            dr = abs(dr); 
            r = target_rs * 1.05; // pop out just outside the horizon
            
            // Re-evaluate Constants for the exact new radius!
            double f = 1.0 - target_rs / r;
            if(f <= 0.0) f = 0.0001;
            double dt_dlam_new = sqrt((dr*dr)/(f*f) + (r*r*dphi*dphi)/f);
            E = f * dt_dlam_new;
            L = r * r * dphi;
            
            // Recalculate x, y for the new mouth
            double new_mx = currentMouth == 0 ? wormhole->mouthA.position.x : wormhole->mouthB.position.x;
            double new_my = currentMouth == 0 ? wormhole->mouthA.position.y : wormhole->mouthB.position.y;
            
            x = new_mx + r * cos(phi);
            y = new_my + r * sin(phi);
            trail.push_back({x, y});
            return;
        }

        rk4Step(dlam, rs);

        x = mx + r * cos(phi);
        y = my + r * sin(phi);

        trail.push_back({x, y});
    }

    void geodesicRHS(double input_r, double input_dr, double input_dphi, double rhs[4], double rs) {
        double f = 1.0 - rs / input_r;
        if(f <= 0.0) f = 0.000001;
        
        rhs[0] = input_dr;
        rhs[1] = input_dphi;
        
        double dt_dlam = E / f;
        rhs[2] = - (rs/(2*input_r*input_r)) * f * (dt_dlam*dt_dlam)
                 + (rs/(2*input_r*input_r*f)) * (input_dr*input_dr)
                 + (input_r - rs) * (input_dphi*input_dphi);
                 
        rhs[3] = -2.0 * input_dr * input_dphi / input_r;
    }

    void addState(const double a[4], const double b[4], double factor, double out[4]) {
        for (int i = 0; i < 4; i++)
            out[i] = a[i] + b[i] * factor;
    }

    void rk4Step(double dlam, double rs) {
        double y0[4] = { r, phi, dr, dphi };
        double k1[4], k2[4], k3[4], k4[4], temp[4];

        geodesicRHS(y0[0], y0[2], y0[3], k1, rs);
        
        addState(y0, k1, dlam/2.0, temp);
        geodesicRHS(temp[0], temp[2], temp[3], k2, rs);

        addState(y0, k2, dlam/2.0, temp);
        geodesicRHS(temp[0], temp[2], temp[3], k3, rs);

        addState(y0, k3, dlam, temp);
        geodesicRHS(temp[0], temp[2], temp[3], k4, rs);

        r    += (dlam/6.0)*(k1[0] + 2*k2[0] + 2*k3[0] + k4[0]);
        phi  += (dlam/6.0)*(k1[1] + 2*k2[1] + 2*k3[1] + k4[1]);
        dr   += (dlam/6.0)*(k1[2] + 2*k2[2] + 2*k3[2] + k4[2]);
        dphi += (dlam/6.0)*(k1[3] + 2*k2[3] + 2*k3[3] + k4[3]);
    }
};

#endif
