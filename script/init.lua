local init = {} 

local timer = 15

function init:update()
  if timer <= 0 then
    return Game.stop_update(self)
  end

  timer = timer - Time.delta
end

function init:stop()
  Log.info("Init script completed.")
end

Game.start_update(init)