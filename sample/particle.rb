# coding: utf-8
# 
# Refer to: tmlib.js - http://phi-jp.github.io/tmlib.js/ 
#                    - http://goo.gl/yhtYLS              Thanks!

require_relative 'sdl_udon'
require 'matrix'

SCREEN_WIDTH, SCREEN_HEIGHT = 640, 480
SCREEN_CENTER_X = SCREEN_WIDTH/2
SCREEN_CENTER_Y = SCREEN_HEIGHT/2

PARTICLE = 10
FRICTION = 0.96
TO_DIST = SCREEN_WIDTH*0.86
STIR_DIST = SCREEN_WIDTH*0.125
BLOW_DIST = SCREEN_WIDTH*0.4

module SDLUdon
  class Vec2D
    def self.rand(min, max, len)
      degree = min + Kernel.rand(max - min)
      rad = (degree+90).radian
      [Math.cos(rad) * len, Math.sin(rad) * len]
    end

    def initialize(*nums, dim: 2)
      @pts = Array.new(dim, 0)
      @pts[0...nums.size] = *nums unless nums.empty?
      @vector = ::Vector.elements(@pts, false)
    end

    def length ; @vector.norm ; end
    def length_squared ; @vector.norm ** 2 ; end
    
    def x ; @pts[0] ; end
    def y ; @pts[1] ; end

    def x=(value) ; @pts[0] = value ; end 
    def y=(value) ; @pts[1] = value ; end 

    def [](index) ; @pts[index] ; end
    def []=(index, value) ; @pts[index] = value ; end

    def add(vector) ; @pts[0] += vector[0]; @pts[1] += vector[1] ; end

    def mul(num) ; @pts[0] *= num ; @pts[1] *= num ; end

    def set_random(min, max, len)
      set_degree(min + rand(max - min), len)
      self
    end

    def set_degree(degree, len)
      rad = (degree+90).radian
      @pts[0], @pts[1] = Math.cos(rad) * len, Math.sin(rad) * len
    end

    def method_missing(name, *args) ; @vector.send(name, *args) ; end

    def +(vector) ; Vec2D.new(self[0] + vector[0], self[1] + vector[1]) ; end
    def -(vector) ; Vec2D.new(self[0] - vector[0], self[1] - vector[1]) ; end
  end
end

module SceneChild
  def update ; ; end
  def active? ; true ; end
end

class Particle
  include SceneChild
  def initialize(h, s, l, a = 1.0)
    rgb = Color.hsl(h, s, l, a)
    @image = Image.new(10,10,rgb)
    @x = @y = 0
    @draw_opts = Struct.new(:center_x, :center_y, 
      :scale_x, :scale_y, :angle).new
    @vector = SDLUdon::Vec2D.new

    @draw_opts.angle = 0
    @draw_opts.scale_x = @draw_opts[:scale_x] = 1.0
    color_blend(:add)
  end
  def draw_opts
    @draw_opts.to_h
  end
  attr_reader :vector
  attr_accessor :x, :y

  def fill_color(color) ; @image.fill(color) ; end

  def color_blend(blend) ; @image.blend = blend ; end
  
  def move(x, y) ; self.x = x  ; self.y = y ; end

  def move_add(add_x, add_y) ; self.x += add_x  ; self.y += add_y ; end

  def update
    dv =  Vector[@x,@y] - Vector[Input.mouse_x, Input.mouse_y]
    d = dv.norm
    d = 0.001 if d.zero?
    dv = Vector[dv[0] / d, dv[1] / d]

    if Input.mouse_down?(:lbtn)
      if d < BLOW_DIST
        blowAcc = (1 - d / BLOW_DIST) * 14
        @vector.x += dv[0] * blowAcc + 0.5 - rand
        @vector.y += dv[1] * blowAcc + 0.5 - rand
      end
    end
    if d < TO_DIST
      toAcc = ( 1 -  d / TO_DIST) * SCREEN_WIDTH * 0.0014
      @vector.x -= dv[0] * toAcc
      @vector.y -= dv[1] * toAcc
    end

    if d < STIR_DIST
      mAcc = ( 1 - d / STIR_DIST) * SCREEN_WIDTH * 0.00026
      @vector.x += Input.mouse_delta_x * mAcc * 0.1
      @vector.y += Input.mouse_delta_y * mAcc * 0.1
    end

    @vector.mul(FRICTION)

    move_add(@vector.x, @vector.y)

    if self.x > SCREEN_WIDTH
      @x = SCREEN_WIDTH
      @vector[0] *= -1
    elsif self.x < 0
      @x = 0
      @vector[0] *= -1
    end

    if self.y > SCREEN_HEIGHT
      @y = SCREEN_HEIGHT
      @vector[1] *= -1
    elsif self.y < 0
      @y = 0
      @vector[1] *= -1
    end

    scale = @vector.length_squared * 0.04
    scale = Math.clamp(scale, 0.75, 2)
    @draw_opts[:scale_x] = @draw_opts[:scale_y] = scale
    @draw_opts[:angle] += (scale*10)
  end
end

Game.loop(fps: 30) { |game|
  game.run {
    PARTICLE.times {
      c = Particle.new(rand(360), 0.75, 0.5, 1.0)
      c.move(*Vec2D.rand(0, 360, rand(SCREEN_CENTER_X)))
      c.move_add(SCREEN_CENTER_X, SCREEN_CENTER_Y)
      add_child(c)
    }
  }
}