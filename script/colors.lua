local Cvar = require("cvar")
local colors = {}

local function randf(a, b)
  return a + (b - a) * math.random()
end

local sun_color = { x = 0, y = 0, z = 0, w = 0 }
local dir = { x = 1, y = 1, z = 1, w = 1 }
local vel = { x = randf(.1, .5), y = randf(.1, .5), z = randf(.1, .5), w = randf(.1, .5) }

function colors:update()

end

Game.start_update(colors)
