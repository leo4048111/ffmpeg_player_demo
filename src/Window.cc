#include "Window.hxx"

#include "Logger.hxx"

namespace fpd
{
    Window::Window() = default;
    Window::~Window() = default;

    bool Window::init(const int windowWidth, const int windowHeight, const int textureWidth, const int textureHeight)
    {
        int ec = 0;

        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
        TTF_Init();

        _window = SDL_CreateWindow("ffmpeg player demo",
                                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   windowWidth, windowHeight,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

        if (_window == nullptr)
        {
            LOG_ERROR("Failed to create SDL window, error: %s", SDL_GetError());
            return -1;
        }

        _renderer = SDL_CreateRenderer(_window, -1, 0);
        _texture = SDL_CreateTexture(_renderer,
                                     SDL_PIXELFORMAT_IYUV,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     textureWidth, textureHeight);

        _windowRect.x = _textureRect.x = 0;
        _windowRect.y = _textureRect.y = 0;
        _windowRect.w = windowWidth;
        _windowRect.h = windowHeight;
        _textureRect.w = textureWidth;
        _textureRect.h = textureHeight;

        // init mem font
#include "FreeSans.inc"
        const int FontSize = 16;

        SDL_RWops *fontMem = SDL_RWFromConstMem(freesans_data, freesans_data_length);

        _font = TTF_OpenFontRW(fontMem, 1, FontSize);

        if (!_font)
            LOG_WARNING("Failed to load font from memory buffer, text render may not be working, error: %s", SDL_GetError());

        _sdlInitialized = true;

        return ec;
    }

    void Window::destroy()
    {
        if (_texture != nullptr)
        {
            SDL_DestroyTexture(_texture);
            _texture = nullptr;
        }

        if (_renderer != nullptr)
        {
            SDL_DestroyRenderer(_renderer);
            _renderer = nullptr;
        }

        if (_window != nullptr)
        {
            SDL_DestroyWindow(_window);
            _window = nullptr;
        }

        if (_font != nullptr)
        {
            TTF_CloseFont(_font);
            _font = nullptr;
        }

        TTF_Quit();
        SDL_Quit();

        _sdlInitialized = false;
    }

    void Window::resize(const int width, const int height)
    {
        if (!_sdlInitialized)
            return;

        // _rect.w = width;
        // _rect.h = height;
    }

    void Window::loop(WindowLoopCallback onWindowLoop)
    {
        _running = true;
        SDL_Event e;
        while (_running)
        {
            onWindowLoop();
            while (SDL_PollEvent(&e))
            {
                if (e.type == SDL_QUIT)
                {
                    _running = false;
                }
            }
        }
    }

    void Window::videoRefresh(const Uint8 *ydata, const int ysize,
                              const Uint8 *udata, const int usize,
                              const Uint8 *vdata, const int vsize)
    {
        if (!_sdlInitialized)
            return;

        SDL_UpdateYUVTexture(_texture, &_textureRect,
                             ydata, ysize,
                             udata, usize,
                             vdata, vsize);

        SDL_RenderClear(_renderer);
        double ratio = _textureRect.w / (double)_textureRect.h;
        SDL_Rect tmpRect;
        if (_textureRect.h > _textureRect.w)
        {
            tmpRect.h = _windowRect.h;
            tmpRect.w = ratio * tmpRect.h;
            tmpRect.x = (_windowRect.w - tmpRect.w) / 2;
            tmpRect.y = 0;
        }
        else if(_textureRect.h < _textureRect.w)
        {
            tmpRect.w = _windowRect.w;
            tmpRect.h = tmpRect.w / ratio;
            tmpRect.x = 0;
            tmpRect.y = (_windowRect.h - tmpRect.h) / 2;
        }
        else {
            if(_windowRect.h < _windowRect.w)
            {
                tmpRect.h = _windowRect.h;
                tmpRect.w = tmpRect.h * ratio;
                tmpRect.x = (_windowRect.w - tmpRect.w) / 2;
                tmpRect.y = 0;
            } 
            else {
                tmpRect.w = _windowRect.w;
                tmpRect.h = tmpRect.w / ratio;
                tmpRect.x = 0;
                tmpRect.y = (_windowRect.h - tmpRect.h) / 2;
            }
        }

        SDL_RenderCopy(_renderer, _texture, &_textureRect, &tmpRect);
    }

    void Window::addText(const std::string &text, const int x, const int y, const SDL_Color &color)
    {
        if (!_sdlInitialized || !_font)
            return;

        SDL_Surface *surface = TTF_RenderText_Solid(_font, text.c_str(), color);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(_renderer, surface);

        SDL_Rect rect;
        rect.x = x;
        rect.y = y;
        rect.w = surface->w;
        rect.h = surface->h;

        SDL_RenderCopy(_renderer, texture, nullptr, &rect);

        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

    void Window::render()
    {
        SDL_RenderPresent(_renderer);
    }

    int Window::openAudio(SDL_AudioSpec spec)
    {
        int ec = 0;
        if ((ec = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, SDL_AUDIO_ALLOW_ANY_CHANGE)) < 2)
            LOG_ERROR("Failed to open audio device, error: %s", SDL_GetError());
        else
            SDL_PauseAudioDevice(ec, 0);

        return ec;
    }

    void Window::closeAudio()
    {
        SDL_CloseAudioDevice(1);
    }
}