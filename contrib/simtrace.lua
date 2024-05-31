-- wireshark LUA dissector for the SIMtrace USB protocol
-- (C) 2021 by sysmocom - s.f.m.c. GmbH, Author: Eric Wild
-- SPDX-License-Identifier: GPL-2.0+
--
-- Usage: Move this file to your "personal lua plugins" folder that
-- can be found in the Wireshark Help->About Wireshark->Folders tab
-- Windows: %APPDATA%\Wireshark\plugins.
-- Unix-like systems: ~/.local/lib/wireshark/plugins.

usb_simtrace_protocol = Proto("USB_simtrace",  "USB simtrace protocol")


local control_commands = {
-- /* SIMTRACE_MSGC_GENERIC */
[0x0000] = "SIMTRACE_CMD_DO_ERROR",
[0x0001] = "SIMTRACE_CMD_BD_BOARD_INFO",

-- /* SIMTRACE_MSGC_CARDEM */
[0x0101] = "DT_CEMU_TX_DATA",
[0x0102] = "DT_CEMU_SET_ATR",
[0x0103] = "BD_CEMU_STATS",
[0x0104] = "BD_CEMU_STATUS",
[0x0105] = "DT_CEMU_CARDINSERT",
[0x0106] = "DO_CEMU_RX_DATA",
[0x0107] = "DO_CEMU_PTS",
[0x0108] = "BD_CEMU_CONFIG",

-- /* SIMTRACE_MSGC_MODEM */
[0x0201] = "DT_MODEM_RESET",
[0x0202] = "DT_MODEM_SIM_SELECT",
[0x0203] = "BD_MODEM_STATUS",

-- /* SIMTRACE_MSGC_SNIFF */
[0x0300] = "SNIFF_CHANGE",
[0x0301] = "SNIFF_FIDI",
[0x0302] = "SNIFF_ATR",
[0x0304] = "SNIFF_TPDU",
[0x0303] = "SNIFF_PPS"
}

local msgtype  = ProtoField.uint16("usb_simtrace.msgtype", "Message Type", base.HEX_DEC, control_commands)
local seqnr  = ProtoField.uint8("usb_simtrace.seqnr", "Sequence Number", base.DEC)
local slotnr  = ProtoField.uint8("usb_simtrace.slotnr", "Slot Number", base.DEC)
local reserved  = ProtoField.uint16("usb_simtrace.reserved", "reserved", base.HEX_DEC)
local payloadlen  = ProtoField.uint16("usb_simtrace.length", "length", base.DEC)
local payload  = ProtoField.bytes("usb_simtrace.payload", "Data")

local pb_and_rx  = ProtoField.uint32("usb_simtrace.pb_and_rx", "pb_and_rx", base.HEX_DEC, NULL, 0x8)
local pb_and_tx  = ProtoField.uint32("usb_simtrace.pb_and_tx", "pb_and_tx", base.HEX_DEC, NULL, 0x4)
local final  = ProtoField.uint32("usb_simtrace.final", "final", base.HEX_DEC, NULL, 0x2)
local tpdu_hdr  = ProtoField.uint32("usb_simtrace.tpdu_hdr", "tpdu_hdr", base.HEX_DEC, NULL, 0x1)
local rxtxdatalen  = ProtoField.uint16("usb_simtrace.rxtxdatalen", "rx/tx data length", base.DEC)
local rxtxdata  = ProtoField.bytes("usb_simtrace.rxtxdata", "rx/tx (data)")

local hf_pts_len = ProtoField.uint8("usb_simtrace.pts_len", "PTS length", base.DEC)
local hf_pts_req = ProtoField.bytes("usb_simtrace.pts_req", "PTS request")
local hf_pts_resp = ProtoField.bytes("usb_simtrace.pts_resp", "PTS response")

local hf_cemu_cfg_features = ProtoField.uint32("usb_simtrace.cemu_cfg.features.status_irq", "CardEm Features", base.HEX)
local hf_cemu_cfg_slot_mux_nr = ProtoField.uint32("usb_simtrace.cemu_cfg.features.slot_mux_nr", "CardEm Slot Mux Nr", base.DEC)
local hf_cemu_cfg_presence_polarity = ProtoField.uint8("usb_simtrace.cemu_cfg.features.presence_polarity", "Sim presence polarity", base.DEC)

local card_insert_types = {
    [0x00] = "not inserted",
    [0x01] = "inserted",
}
local hf_cemu_cardinsert = ProtoField.uint8("usb_simtrace.cardinsert", "Card Insert", base.DEC, card_insert_types, 0xff)

local CEMU_STATUS_F_VCC_PRESENT  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_VCC_PRESENT", "VCC_PRESENT", base.HEX_DEC, NULL, 0x00000001)
local CEMU_STATUS_F_CLK_ACTIVE  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_CLK_ACTIVE", "CLK_ACTIVE", base.HEX_DEC, NULL, 0x00000002)
local CEMU_STATUS_F_RCEMU_ACTIVE  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_RCEMU_ACTIVE", "CEMU_ACTIVE", base.HEX_DEC, NULL, 0x00000004)
local CEMU_STATUS_F_CARD_INSERT  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_CARD_INSERT", "CARD_INSERT", base.HEX_DEC, NULL, 0x00000008)
local CEMU_STATUS_F_RESET_ACTIVE  = ProtoField.uint32("usb_simtrace.CEMU_STATUS.F_RESET_ACTIVE", "RESET_ACTIVE", base.HEX_DEC, NULL, 0x00000010)

local CEMU_CONFIG_PRES_POL_PRES_H  = ProtoField.uint32("usb_simtrace.CEMU_CONFIG.PRES_POL_PRES_H", "PRESENCE_HIGH", base.HEX_DEC, NULL, 0x00000001)
local CEMU_CONFIG_PRES_POL_VALID  = ProtoField.uint32("usb_simtrace.CEMU_CONFIG.PRES_POL_VALID", "PRESENCE_VALID", base.HEX_DEC, NULL, 0x00000002)

local modem_reset_types = {
    [0x00] = "de-assert",
    [0x01] = "assert",
    [0x02] = "pulse"
}
local modem_reset_status = ProtoField.uint8("usb_simtrace.modem.reset_type", "modem reset type", base.HEX, modem_reset_types, 0xf)
local modem_reset_len = ProtoField.uint8("usb_simtrace.modem.reset_len", "modem reset length (ms)", base.DEC)

local modem_sim_select_types = {
    [0x00] = "local",
    [0x01] = "remote",
}
local hf_modem_sim_select = ProtoField.uint8("usb_simtrace.modem.sim_select", "SIM card selection", base.DEC, modem_sim_select_types, 0xff)

usb_simtrace_protocol.fields = {
  msgtype, seqnr, slotnr, reserved, payloadlen, payload,
  pb_and_rx, pb_and_tx, final, tpdu_hdr, rxtxdatalen, rxtxdata,
  CEMU_STATUS_F_VCC_PRESENT, CEMU_STATUS_F_CLK_ACTIVE, CEMU_STATUS_F_RCEMU_ACTIVE, CEMU_STATUS_F_CARD_INSERT, CEMU_STATUS_F_RESET_ACTIVE,
  CEMU_CONFIG_PRES_POL_PRES_H, CEMU_CONFIG_PRES_POL_VALID,
  modem_reset_status, modem_reset_len,
  hf_pts_len, hf_pts_req, hf_pts_resp,
  hf_cemu_cfg_features, hf_cemu_cfg_slot_mux_nr, hf_cemu_cfg_presence_polarity,
  hf_cemu_cardinsert, hf_modem_sim_select,
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

  local flagstr = " "
  if is_pbrx().value == 1 then
    flagstr = flagstr .. "R"
  else
    flagstr = flagstr .. "."
  end
  if is_pbtx().value == 1 then
    flagstr = flagstr .. "T"
  else
    flagstr = flagstr .. "."
  end
  if is_final().value == 1 then
    flagstr = flagstr .. "F"
  else
    flagstr = flagstr .. "."
  end
  if is_hdr().value == 1 then
    flagstr = flagstr .. "H"
  else
    flagstr = flagstr .. "."
  end
  flagstr = flagstr .. " "
  pinfo.cols.info:append(flagstr .. payload_data(6,len))

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

function dissect_pts(payload_data, pinfo, tree)
  local subtree = tree:add(usb_simtrace_protocol, payload_data, "PTS")
  local pts_len = payload_data(0,1):le_uint()
  local pts_req = payload_data(1, pts_len);
  local pts_resp = payload_data(7, pts_len);

  subtree:add(hf_pts_len, pts_len);
  subtree:add(hf_pts_req, pts_req);
  subtree:add(hf_pts_resp, pts_resp);

  pinfo.cols.info:append(" Req: " .. pts_req .. ", Resp: " .. pts_resp);
end

function dissect_cemu_config(payload_data, pinfo, tree)
  local subtree = tree:add(usb_simtrace_protocol, payload_data, "Card Emu Config")

  subtree:add(hf_cemu_cfg_features, payload_data(0,4));
  if payload_data:len() >= 4 then
	subtree:add(hf_cemu_cfg_slot_mux_nr, payload_data(4,1));
  end
  if payload_data:len() >= 5 then
	local pres =  payload_data(5,1):le_uint();
    subtree:add(hf_cemu_cfg_presence_polarity, payload_data(5,1));
	headerSubtree:add(CEMU_CONFIG_PRES_POL_PRES_H, pres)
	headerSubtree:add(CEMU_CONFIG_PRES_POL_VALID, pres)
  end
end

function dissect_modem_sim_sel(payload_data, pinfo, tree)
  local subtree = tree:add(usb_simtrace_protocol, payload_data, "Modem SIM Select")
  local sim_select = payload_data(0,1):le_uint();

  subtree:add(hf_modem_sim_select, sim_select);
  pinfo.cols.info:append(" " .. modem_sim_select_types[sim_select]);
end

function dissect_cemu_cardinsert(payload_data, pinfo, tree)
  local subtree = tree:add(usb_simtrace_protocol, payload_data, "Card Insert")
  local cins_type = payload_data(0,1):le_uint()

  subtree:add(hf_cemu_cardinsert, cins_type);
  pinfo.cols.info:append(" " .. card_insert_types[cins_type]);
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
  elseif(command == 0x0105) then
    return dissect_cemu_cardinsert(payload_data(),pinfo,subtree)
  elseif(command == 0x0107) then
    return dissect_pts(payload_data(),pinfo,subtree)
  elseif(command == 0x0108) then
    return dissect_cemu_config(payload_data(),pinfo,subtree)
  elseif(command == 0x0201) then
    return dissect_modem_reset(payload_data(),pinfo,subtree)
  elseif(command == 0x0202) then
    return dissect_modem_sim_sel(payload_data(),pinfo,subtree)
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
  DissectorTable.get("usb.device"):add_for_decode_as(usb_simtrace_protocol)
  DissectorTable.get("usb.bulk"):add(0xffff, usb_simtrace_protocol)
  DissectorTable.get("usb.interrupt"):add(0xffff, usb_simtrace_protocol)
end
