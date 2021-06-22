local Cvar = require("cvar")

local init = {}

local shutter = 0
local speed = 0.01

function init:start()
  Cvar.set("exp_manual", 1)
  Cvar.set("exp_shutter", 0)
end

function init:update()  
  shutter = shutter + speed * Time.delta
  if (shutter > 0.059) then
    return Game.stop_update(self)
  end
  
  Cvar.set("exp_shutter", shutter)
end

function init:stop()
  Log.info("Init script completed.")
end

Game.start_update(init)