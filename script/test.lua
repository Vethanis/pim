local test = {}

local timer = 1
local repeats = 5

function test:start()
  Log.info("Beginning test...")
end

function test:update()
  if timer <= 0 then
    repeats = repeats - 1
    timer = 1
    
    self:tick()

    if repeats <= 0 then
      return Game.stop_update(self)
    end
  end

  timer = timer - Time.delta
end

function test:stop()
  Log.info("Test script completed.")
end

function test:tick()
    Log.info(string.format("TICK! %i secs remain", repeats))
end

Game.start_update(test)