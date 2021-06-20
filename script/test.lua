local Cmd = require("cmd")
local Cvar = require("cvar")

local test = {}

local timer = 1
local repeats = 5
local dir = 1
local fov = 90
local speed = 5

function test:start()
  Log.info("Beginning test...")
  fov = Cvar.get("r_fov")
  Log.info("Current fov is ", fov)
end

function test:update()
  if timer <= 0 then
    repeats = repeats - 1
    timer = 1
    
    dir = -dir

    if repeats <= 0 then
      return Game.stop_update(self)
    end
  end
  
  Cvar.set("r_fov", fov)
  fov = fov + (dir * speed * Time.delta)

  timer = timer - Time.delta
end

function test:stop()
  Log.info("Test script completed.")
end

Game.start_update(test)