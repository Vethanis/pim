local Cvar = require("cvar")

local vector_test = {}

function vector_test:update()
  for i=1, 10000 do
    local sun_vector = Cvar.get("r_sun_col")
  end
end

Game.start_update(vector_test);