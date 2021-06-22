local Cvar = require("cvar")
local colors = {}

local function randf(a, b)
  local r = math.random()
  return (r + a) * (b - a)
end

local sun_color = { x = 0, y = 0, z = 0, w = 0 }
local dir = { x = 1, y = 1, z = 1, w = 1 }
local vel = { x = randf(.1, .5), y = randf(.1, .5), z = randf(.1, .5), w = randf(.1, .5) }

function colors:update()
  for k, v in pairs(sun_color) do
    if v >= 1 or v <= 0 then
      dir[k] = -dir[k]
      vel[k] = randf(.1, .5)
      if v > 1 then
        sun_color[k] = 1
      else
        sun_color[k] = 0
      end
    end
    
    sun_color[k] = sun_color[k] + dir[k] * vel[k] * Time.delta
  end

  Cvar.set("r_sun_col", sun_color)
end

Game.start_update(colors)
