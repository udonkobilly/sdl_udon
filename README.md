# SDLUdon
## 2D Game Library Ruby binding for SDL

# Ready


- ruby 2.2.2-i386-mingw32 ( http://rubyinstaller.org/ )

- sdl 2.0.3 ( https://www.libsdl.org/ )

  sdl_image 2.0 ( https://www.libsdl.org/projects/SDL_image/ )

  sdl_mixer 2.0 ( https://www.libsdl.org/projects/SDL_mixer/ )

  sdl_ttf 2.0 ( https://www.libsdl.org/projects/SDL_ttf/ )

# Compile
    git clone https://github.com/udonkobilly/sdl_udon.git or
    https://github.com/udonkobilly/sdl_udon/archive/master.zip
    cd sdl_udon
(gcc -MM *.c > depend)

    ruby extconf.rb
    make
    make site-install