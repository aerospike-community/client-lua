
local as = require "as_lua"

local function main()
  local record
  local err
  local message
  local cluster
  
  err, message, cluster = as.connect("localhost", 3000)
  print("connected", err, message)
  
  local bins = {}
  bins["uid"] = "peter001"
  bins["name"] = "Peter"
  bins["dob"] = 19800101
  
  err, message = as.put(cluster, "test", "test", "peter001", 3, bins)
  print("saved record", err, message)
  
  err, message, record = as.get(cluster, "test", "test", "peter001")
  print("read record", err, message, record.uid, record.name, record.dob)

  bins["uid"] = "peter002"
  bins["name"] = "Peter2"
  bins["dob"] = 19800102
  
  err, message = as.put(cluster, "test", "test", "peter002", 3, bins)
  print("saved record", err, message)
  
  bins = {}
  bins["counter"] = 1
  err, message = as.increment(cluster, "test", "test", "peter003", 1, bins)
  print("incremented record", err, message)
  
  err, message, record = as.get(cluster, "test", "test", "peter003")
  print("read record", err, message, record.counter)

  
  err, message = as.disconnect(cluster)
  print("disconnected", err, message)
  
end
main()
