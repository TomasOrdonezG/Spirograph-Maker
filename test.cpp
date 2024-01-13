#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 300

struct Ball
{
    float x, y;
    float vx, vy;
    int r;

    Ball():
        x( SCREEN_WIDTH / 2.0f),
        y( SCREEN_HEIGHT / 2.0f),
        vx(1e-1),
        vy(2e-1),
        r(5)
    {}
};

int SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius);

int main(int argc, char **argv)
{
    // Initialize
    Ball ball;

    SDL_Window *window = SDL_CreateWindow("Text", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    bool running = true;
    int frames = 0;
    SDL_Event event;
    while (running)
    {
        frames++;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE))
                running = 0;

        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (frames % 5000 == 0)
        {
            SDL_SetRenderTarget(renderer, texture);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_SetRenderTarget(renderer, NULL);
        }


        // Move and bounce ball
        ball.x += ball.vx;
        ball.y += ball.vy;
        if (ball.x - ball.r <= 0 || ball.x + ball.r >= SCREEN_WIDTH) ball.vx *= -1;
        if (ball.y - ball.r <= 0 || ball.y + ball.r >= SCREEN_HEIGHT) ball.vy *= -1;

        SDL_SetRenderTarget(renderer, texture);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawPoint(renderer, ball.x, ball.y);
        SDL_SetRenderTarget(renderer, NULL);
        SDL_RenderCopy(renderer, texture, NULL, NULL);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderFillCircle(renderer, ball.x, ball.y, ball.r);

        SDL_RenderPresent(renderer);
    }    

    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_Quit();
    
    return 0;
}

int SDL_RenderFillCircle(SDL_Renderer *renderer, int x, int y, int radius)
{
    // Credit: https://gist.github.com/Gumichan01/332c26f6197a432db91cc4327fcabb1c
    int offsetx, offsety, d;
    int status;

    offsetx = 0;
    offsety = radius;
    d = radius -1;
    status = 0;

    while (offsety >= offsetx)
    {
        status += SDL_RenderDrawLine(renderer, x - offsety, y + offsetx, x + offsety, y + offsetx);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y + offsety, x + offsetx, y + offsety);
        status += SDL_RenderDrawLine(renderer, x - offsetx, y - offsety, x + offsetx, y - offsety);
        status += SDL_RenderDrawLine(renderer, x - offsety, y - offsetx, x + offsety, y - offsetx);

        if (status < 0) 
        {
            status = -1;
            break;
        }

        if (d >= 2*offsetx)
        {
            d -= 2*offsetx + 1;
            offsetx +=1;
        }
        else if (d < 2 * (radius - offsety))
        {
            d += 2 * offsety - 1;
            offsety -= 1;
        }
        else
        {
            d += 2 * (offsety - offsetx - 1);
            offsety -= 1;
            offsetx += 1;
        }
    }

    return status;
}

