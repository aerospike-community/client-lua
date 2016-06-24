
local as = require "as_lua"

function tprint (tbl, indent)
  if not indent then indent = 0 end
  for k, v in pairs(tbl) do
    formatting = string.rep("  ", indent) .. k .. ": "
    if type(v) == "table" then
      print(formatting)
      tprint(v, indent+1)
    else
      print(formatting .. v)
    end
  end
end


local function main()
  local record
  local err
  local message
  local cluster
  
  err, message, cluster = as.connect("127.0.0.1", 3000)
  print("## connected ##", err, message)
  
  local bins = {}
  bins["uid"] = "peter001"
  bins["name"] = "Peter"
  bins["dob"] = 19800101
  
  err, message = as.put(cluster, "test", "test", "peter001", 3, bins)
  print("saved record peter001", err, message)
  
  err, message, record = as.get(cluster, "test", "test", "peter001")
  print("read record peter001", err, message)
  tprint(record, 1)

  bins = {}
  bins["uid"] = "peter002"
  bins["name"] = "Peter2"
  bins["dob"] = 19800102
  
  err, message = as.put(cluster, "test", "test", "peter002", 3, bins)
  print("saved record peter002", err, message)
 
  bins = {}
  bins["uid"] = "peter004"
  bins["name"] = "Peter4"
  bins["dob"] = 19800102
  bins["animals"] = {"cats","mice","dogs","elephants","snakes"}
  
  err, message = as.put(cluster, "test", "test", "peter004", 4, bins)
  print("saved record peter004", err, message)
  
  bins = {}
  bins["counter"] = 1
  err, message = as.increment(cluster, "test", "test", "peter003", 1, bins)
  print("incremented record", err, message)
  
  err, message, record = as.get(cluster, "test", "test", "peter003")
  print("read record", err, message)
  tprint(record, 1)
  
  err, message, record = as.get(cluster, "test", "test", "peter004")
  print("read record", err, message)
  tprint(record, 1)

  bins = {}
  bins["uid"] = "peter005"
  bins["name"] = "Peter5"
  bins["dob"] = 19800102
  bins["animals"] = {"cats","mice","dogs","elephants","snakes"}
  bins["weight"] = 72.5

  err, message = as.put(cluster, "test", "test", "peter005", 5, bins)
  print("saved record peter005", err, message)

  err, message, record = as.get(cluster, "test", "test", "peter005")
  print("read record", err, message)
  tprint(record, 1)
  
  err, message = as.disconnect(cluster)
  print("disconnected", err, message)
  
end
main()
