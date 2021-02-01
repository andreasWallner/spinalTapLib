spinaltap_v1_protocol = Proto("SPT1", "SpinalTap USB v1")

source = ProtoField.uint8("spt.source", "source", base.HEX)
opcode = ProtoField.uint8("spt.opcode", "opcode", base.HEX)
address = ProtoField.uint16("spt.address", "address", base.HEX)
value = ProtoField.uint32("spt.value", "value", base.HEX)

spinaltap_v1_protocol.fields = {source, opcode, address, value}

function spinaltap_v1_protocol.dissector(buffer, pinfo, tree)
  pinfo.cols.protocol = spinaltap_v1_protocol.name
  dissect(buffer(), pinfo, tree, 20)
end

function dissect(buffer, pinfo, tree, recurse)
  length = buffer:len()
  if (length == 0) or (recurse == 0) then return end

  local subtree = tree:add(spinaltap_v1_protocol, buffer(0, oplen), "SpinalTap")
  if (length < 2) then
    subtree:add_expert_info(PI_MALFORMED, PI_ERROR, "insufficient length")
    return
  end

  local op = buffer(1, 1):int()
  local oplen = get_opcode_length(op)
  local opname = get_opcode_name(op)
  
  subtree:append_text(" " .. opname)
  
  if length < oplen then 
    subtree:add_expert_info(PI_MALFORMED, PI_ERROR, "insufficient length")
    return
  end

  subtree:add_le(source, buffer(0,1))
  subtree:add_le(opcode, buffer(1,1)):append_text(" (" .. opname .. ")")

  if op == 2 then
    subtree:add_le(address, buffer(2, 2))
    length = length - 4
  elseif op == 1 then
    subtree:add_le(address, buffer(2, 2))
    subtree:add_le(value, buffer(4, 4))
    length = length - 8
  else
    subtree:add_expert_info(PI_MALFORMED, PI_ERROR, "invalid opcode")
    return
  end

  if buffer:len() > oplen then
    dissect(buffer(oplen), pinfo, tree, recurse - 1)
  end
end

function get_opcode_name(opcode)
  local opcode_name = "invalid"

  if opcode == 1 then opcode_name = "write"
  elseif opcode == 2 then opcode_name = "read"
  end

  return opcode_name
end

function get_opcode_length(opcode)
  if opcode == 1 then return 8
  elseif opcode == 2 then return 4
  end
  return 0
end

DissectorTable.get("usb.product"):add(0x12091337, spinaltap_v1_protocol)
