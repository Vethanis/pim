
local timer = 5;

function update()
  timer = timer - Time.delta
  Log.info("tick ", timer)
  
  if timer < 0 then
    Script.exit()
  end
end

Log.info("Script system has been initialized.")