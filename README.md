# 2D Game Library Ruby binding for SDL

@ requirements


- ruby 2.2.2-i386-mingw32 ( http://rubyinstaller.org/ )

- sdl 2.0.3 ( https://www.libsdl.org/ )

  sdl_image 2.0 ( https://www.libsdl.org/projects/SDL_image/ )

  sdl_mixer 2.0 ( https://www.libsdl.org/projects/SDL_mixer/ )

  sdl_ttf 2.0 ( https://www.libsdl.org/projects/SDL_ttf/ )

  
@ compile

(gcc -MM *.c > depend)

    ruby extconf.rb
    make
    make site-install