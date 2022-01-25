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

local CEMU_STATUS_F_VCC_PRESENT  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_VCC_PRESENT", "VCC_PRESENT", base.HEX_DEC, NULL, 0x00000001)
local CEMU_STATUS_F_CLK_ACTIVE  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_CLK_ACTIVE", "CLK_ACTIVE", base.HEX_DEC, NULL, 0x00000002)
local CEMU_STATUS_F_RCEMU_ACTIVE  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_RCEMU_ACTIVE", "CEMU_ACTIVE", base.HEX_DEC, NULL, 0x00000004)
local CEMU_STATUS_F_CARD_INSERT  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_CARD_INSERT", "CARD_INSERT", base.HEX_DEC, NULL, 0x00000008)
local CEMU_STATUS_F_RESET_ACTIVE  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_RESET_ACTIVE", "RESET_ACTIVE", base.HEX_DEC, NULL, 0x00000010)

local modem_reset_types = {
    [0x00] = "de-assert",
    [0x01] = "assert",
    [0x02] = "pulse"
}
local modem_reset_status = ProtoField.uint8("usb_simtrace.modem.reset_type", "modem reset type", base.HEX, modem_reset_types, 0xf)
local modem_reset_len = ProtoField.uint8("usb_simtrace.modem.reset_len", "modem reset length (ms)", base.DEC)

usb_simtrace_protocol.fields = {
  msgtype, seqnr, slotnr, reserved, payloadlen, payload,
  pb_and_rx, pb_and_tx, final, tpdu_hdr, rxtxdatalen, rxtxdata,
  CEMU_STATUS_F_VCC_PRESENT, CEMU_STATUS_F_CLK_ACTIVE, CEMU_STATUS_F_RCEMU_ACTIVE, CEMU_STATUS_F_CARD_INSERT, CEMU_STATUS_F_RESET_ACTIVE,
  modem_reset_status, modem_reset_len
}

local is_hdr = Field.new("usb_simtrace.tpdu_hdr")
local is_pbrx = Field.new("usb_simtrace.pb_and_rx")
local is_pbtx = Field.new("usb_simtrace.pb_and_tx")
local is_final= Field.new("usb_simtrace.final")

function dissect_rxtx(payload_data,pinfo,tree)

  local headerSubtree = tree:add(usb_simtrace_protocol, payload_data, "rx/tx data")
  local len  = payload_data(4,2):le_uint();
  local cmd32 =  payload_data(0,4):le_uint();

  headerSubtree:add(pb_and_rx, cmd32)
  headerSubtree:add(pb_and_tx, cmd32)
  headerSubtree:add(final, cmd32)
  headerSubtree:add(tpdu_hdr, cmd32)

  headerSubtree:add(rxtxdatalen, len)
  headerSubtree:add_le(rxtxdata, payload_data(6,len))

  -- ghetto dissection does not work due to mixed in procedure bytes
  --if pinfo.visited == false then
  -- Dissector.get("iso7816"):call(payload_data(6):tvb(), pinfo, tree)

-- local offs = 0
-- if (is_pbrx().value == 1 or is_pbtx().value == 1) and is_final().value == 0 then
--    offs = 1
-- else
--  offs = 0
-- end
--
-- if is_hdr().value == 1 then
--   Dissector.get("gsm_sim"):call(concatss:tvb(), pinfo, tree)
--   concatss =  payload_data(6):bytes()
--  else
--   concatss = concatss .. payload_data(6+offs):bytes()
--  end

--end

end

function dissect_status(payload_data,pinfo,tree)

  local headerSubtree = tree:add(usb_simtrace_protocol, payload_data, "status message")
  local cmd32 =  payload_data(0,4):le_uint();

  headerSubtree:add(CEMU_STATUS_F_VCC_PRESENT, cmd32)
  headerSubtree:add(CEMU_STATUS_F_CLK_ACTIVE, cmd32)
  headerSubtree:add(CEMU_STATUS_F_RCEMU_ACTIVE, cmd32)
  headerSubtree:add(CEMU_STATUS_F_CARD_INSERT, cmd32)
  headerSubtree:add(CEMU_STATUS_F_RESET_ACTIVE, cmd32)

  pinfo.cols.info:append(" VCC:" .. payload_data(0,1):bitfield(7, 1) .. " CLK:" .. payload_data(0,1):bitfield(6, 1) .. " RESET:" .. payload_data(0,1):bitfield(3, 1))
end

function dissect_atr(payload_data,pinfo,tree)

  local len  = payload_data(0,1):le_uint()
  Dissector.get("iso7816.atr"):call(payload_data(1):tvb(), pinfo, tree)
end

function dissect_modem_reset(payload_data,pinfo,tree)

  local headerSubtree = tree:add(usb_simtrace_protocol, payload_data, "modem reset")
  local cmd8 =  payload_data(0,1):le_uint();

  headerSubtree:add(modem_reset_status, cmd8)
  pinfo.cols.info:append(" reset type:" .. modem_reset_types[cmd8]);

  if(cmd8 == 2) then
    local duration = payload_data(1,2):le_uint()
    headerSubtree:add(modem_reset_len, duration)
    pinfo.cols.info:append(" duration:" ..  duration .. "ms")
  end


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
    return dissect_rxtx(payload_data(),pinfo,subtree)
  elseif(command == 0x0104) then
	return dissect_status(payload_data(),pinfo,subtree)
  elseif(command == 0x0102) then
	return dissect_atr(payload_data(),pinfo,subtree)
  elseif(command == 0x0201) then
	return dissect_modem_reset(payload_data(),pinfo,subtree)
  else
    subtree:add(payload, payload_data)
  end

end


function usb_simtrace_protocol.init()
local usb_product_dissectors = DissectorTable.get("usb.product")
usb_product_dissectors:add(0x1d50616d, usb_simtrace_protocol) -- OCTSIMTEST
usb_product_dissectors:add(0x1d50616e, usb_simtrace_protocol) -- NGFF_CARDEM
usb_product_dissectors:add(0x1d5060e3, usb_simtrace_protocol) -- SIMTRACE2
usb_product_dissectors:add(0x1d504004, usb_simtrace_protocol) -- QMOD
usb_product_dissectors:add(0x1d504001, usb_simtrace_protocol) -- OWHW
DissectorTable.get("usb.bulk"):add(0xffff, usb_simtrace_protocol)
DissectorTable.get("usb.interrupt"):add(0xffff, usb_simtrace_protocol)
--concatss =  ByteArray.new()
end
