require "mkmf"

case RUBY_PLATFORM
when /i386-mingw32/
  require 'devkit'
  dir_config('sdl')
  libs = %w[mingw32 SDL2main SDL2 SDL2_image SDL2_ttf SDL2_mixer shell32 ws2_32 iphlpapi imagehlp shlwapi]
  libs.each { |i| raise "NotFound [#{i}]..." unless have_library(i) }

  # system("gcc -MM *.c > depend") unless File.exist?('depend')
  optflags = '-mwindows -mconsole -fno-omit-frame-pointer -fno-fast-math'
  # debugflags = -g -O0 -m32
  releaseflags = '-s -O2 -m32'
  $CFLAGS << ' ' << optflags << ' ' << releaseflags
  $LIBPATH <<[ File.dirname(find_executable("ruby")) ]
  $LDFLAGS << ' ' << "-m32"
  $CPPFLAGS << ' ' << "-I#{$hdrdir} -I#{$hdrdir}/i386-mingw32"

  create_makefile('sdl_udon')

  open(defpath = "sdl_udon-i386-mingw32.def", "w") { |f|
    f.write "EXPORTS\nInit_sdl_udon"
  }
else
  raise "Sorry! ruby-i386-mingw32 only..."
end