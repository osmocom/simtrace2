usb_simtrace_protocol = Proto("USB_simtrace",  "USB simtrace protocol")


local control_commands = {
-- /* SIMTRACE_MSGC_GENERIC */
[0x0000] = "SIMTRACE_CMD_DO_ERROR",
[0x0001] = "SIMTRACE_CMD_BD_BOARD_INFO",

-- /* SIMTRACE_MSGC_CARDEM */
[0x0101] = "SIMTRACE_MSGT_DT_CEMU_TX_DATA",
[0x0102] = "SIMTRACE_MSGT_DT_CEMU_SET_ATR",
[0x0103] = "SIMTRACE_MSGT_BD_CEMU_STATS",
[0x0104] = "SIMTRACE_MSGT_BD_CEMU_STATUS",
[0x0105] = "SIMTRACE_MSGT_DT_CEMU_CARDINSERT",
[0x0106] = "SIMTRACE_MSGT_DO_CEMU_RX_DATA",
[0x0107] = "SIMTRACE_MSGT_DO_CEMU_PTS",
[0x0108] = "SIMTRACE_MSGT_BD_CEMU_CONFIG",

-- /* SIMTRACE_MSGC_MODEM */
[0x0201] = "SIMTRACE_MSGT_DT_MODEM_RESET",
[0x0202] = "SIMTRACE_MSGT_DT_MODEM_SIM_SELECT",
[0x0203] = "SIMTRACE_MSGT_BD_MODEM_STATUS",

-- /* SIMTRACE_MSGC_SNIFF */
[0x0300] = "SIMTRACE_MSGT_SNIFF_CHANGE",
[0x0301] = "SIMTRACE_MSGT_SNIFF_FIDI",
[0x0302] = "SIMTRACE_MSGT_SNIFF_ATR",
[0x0304] = "SIMTRACE_MSGT_SNIFF_TPDU",
[0x0303] = "SIMTRACE_MSGT_SNIFF_PPS"
}

local msgtype  = ProtoField.uint16("usb_simtrace.msgtype", "Message Type", base.HEX_DEC, control_commands)
local seqnr  = ProtoField.uint8("usb_simtrace.seqnr", "Sequence Number", base.HEX_DEC)
local slotnr  = ProtoField.uint8("usb_simtrace.slotnr", "Slot Number", base.HEX_DEC)
local reserved  = ProtoField.uint16("usb_simtrace.reserved", "reserved", base.HEX_DEC)
local payloadlen  = ProtoField.uint16("usb_simtrace.length", "length", base.HEX_DEC)
local payload  = ProtoField.bytes("usb_simtrace.payload", "Data")

local pb_and_rx  = ProtoField.uint32("usb_simtrace.pb_and_rx", "pb_and_rx", base.HEX_DEC, NULL, 0x8)
local pb_and_tx  = ProtoField.uint32("usb_simtrace.pb_and_tx", "pb_and_tx", base.HEX_DEC, NULL, 0x4)
local final  = ProtoField.uint32("usb_simtrace.final", "final", base.HEX_DEC, NULL, 0x2)
local tpdu_hdr  = ProtoField.uint32("usb_simtrace.tpdu_hdr", "tpdu_hdr", base.HEX_DEC, NULL, 0x1)
local rxtxdatalen  = ProtoField.uint16("usb_simtrace.rxtxdatalen", "rx/tx data length", base.HEX_DEC)
local rxtxdata  = ProtoField.bytes("usb_simtrace.rxtxdata", "rx/tx (data)")
usb_simtrace_protocol.fields = {
  msgtype, seqnr, slotnr, reserved, payloadlen, payload, pb_and_rx, pb_and_tx, final, tpdu_hdr, rxtxdatalen, rxtxdata
}
function dissect_rxtx(payload_data,pinfo,tree)

  local headerSubtree = tree:add(usb_simtrace_protocol, payload_data, "rx/tx data")
  local len  = payload_data(8+4,2):le_uint();
  local cmd32 =  payload_data(8+0,4):le_uint();

  headerSubtree:add(pb_and_rx, cmd32)
  headerSubtree:add(pb_and_tx, cmd32)
  headerSubtree:add(final, cmd32)
  headerSubtree:add(tpdu_hdr, cmd32)

  headerSubtree:add(rxtxdatalen, len)
  headerSubtree:add_le(rxtxdata, payload_data(8+6,len))
end

function usb_simtrace_protocol.dissector(buffer, pinfo, tree)
  length = buffer:len()
  if length == 0 then return end

  pinfo.cols.protocol = usb_simtrace_protocol.name

  local subtree = tree:add(usb_simtrace_protocol, buffer(), "USB simtrace Data")
  local command = buffer(0,2):uint()

  subtree:add(msgtype, command):set_generated()
  subtree:add(seqnr, buffer(2,1))
  subtree:add(slotnr, buffer(3,1))
  subtree:add_le(payloadlen, buffer(6,2))
  pinfo.cols.info = string.format("Cmd 0x%04X : %s", command, control_commands[command])
  local payload_data = buffer(8,length-8)
  if(command == 0x0101 or command == 0x0106) then
    return dissect_rxtx(buffer(),pinfo,subtree)
  else
    subtree:add(payload, payload_data)
  end

end


function usb_simtrace_protocol.init()
local usb_product_dissectors = DissectorTable.get("usb.product")
usb_product_dissectors:add(0x1d50616d, usb_simtrace_protocol)
usb_product_dissectors:add(0x1d50616e, usb_simtrace_protocol)

-- DissectorTable.get("usb.bulk"):add(0xffff, usb_simtrace_protocol)
end
