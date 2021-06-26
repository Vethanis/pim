local Cvar = require("cvar")

local init = {}

function init:start()

end

function init:update()

end

function init:stop()
  Log.info("Init script completed.")
end

Game.start_update(init)
