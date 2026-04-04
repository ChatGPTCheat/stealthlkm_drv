use bytemuck::{Pod, Zeroable};
use capstone::prelude::*;
use eframe::egui;
use std::io::{Read, Write};
use std::net::TcpStream;
use std::sync::mpsc;
use std::time::{Duration, Instant};
pub mod gdb_server;

const CMD_GET_BASE: u32 = 1;
const CMD_READ_MEM: u32 = 2;
const CMD_SET_HWBP: u32 = 3;
const CMD_CHECK_DEBUG_EVENT: u32 = 4;
const CMD_RESUME_HWBP: u32 = 5;
const CMD_REMOVE_HWBP: u32 = 6;

#[repr(C, packed)]
#[derive(Clone, Copy, Pod, Zeroable, Default)]
struct NetCommand {
    cmd_type: u32,
    address: u64,
    size: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Pod, Zeroable, Default)]
pub struct StealthPtRegs {
    pub regs: [u64; 31],
    pub sp: u64,
    pub pc: u64,
    pub pstate: u64,
}

pub enum NetRequest {
    GetBase(String, mpsc::Sender<u64>),
    ReadMem(u64, u32, mpsc::Sender<Vec<u8>>),
    SetHwbp(u64, mpsc::Sender<bool>),
    RemoveHwbp(mpsc::Sender<bool>),
    ResumeHwbp(StealthPtRegs, mpsc::Sender<()>),
    GdbGetRegisters(mpsc::Sender<Option<StealthPtRegs>>),
}

pub enum UiNotify {
    Error(String),
    BaseResult(u64),
    MemResult(u64, Vec<u8>),
    HwbpSetResult(bool),
    HwbpTriggered(StealthPtRegs),
    HwbpResumed,
}

struct StealthApp {
    tx: mpsc::Sender<UiNotify>,
    net_tx: mpsc::Sender<NetRequest>,
    ui_rx: mpsc::Receiver<UiNotify>,

    // UI States
    target_module: String,
    base_address: u64,

    // Memory Inspector State
    read_addr_str: String,
    read_size_str: String,
    last_read_addr: u64,
    raw_memory: Vec<u8>,
    disasm_text: String,
    memory_tab: usize,       // 0 = Hex/Pointers, 1 = Disasm
    history_stack: Vec<u64>, // Breadcrumbs for pointer jumping

    // HWBP State
    hwbp_addr_str: String,
    hwbp_active: bool,
    frozen_regs: Option<StealthPtRegs>,

    status_msg: String,
    capstone: Option<capstone::Capstone>,
}

impl StealthApp {
    fn new(_cc: &eframe::CreationContext<'_>) -> Self {
        let cs = Capstone::new()
            .arm64()
            .mode(arch::arm64::ArchMode::Arm)
            .detail(false)
            .build()
            .ok();
        let (net_tx, net_rx) = mpsc::channel();
        let (ui_tx, ui_rx) = mpsc::channel();

        let net_tx_clone = net_tx.clone();
        let ui_tx_clone = ui_tx.clone();

        std::thread::spawn(move || {
            network_worker(net_rx, ui_tx);
        });

        // Initialize GDB Server Thread
        let gdb_net_tx = net_tx_clone.clone();
        std::thread::spawn(move || {
            crate::gdb_server::run_gdb_server(gdb_net_tx);
        });

        Self {
            tx: ui_tx_clone,
            net_tx: net_tx_clone,
            ui_rx,
            target_module: "libUE4.so".to_string(),
            base_address: 0,

            read_addr_str: "".to_string(),
            read_size_str: "128".to_string(), // Read 128 bytes default for good view
            last_read_addr: 0,
            raw_memory: Vec::new(),
            disasm_text: "Waiting for read...".to_string(),
            memory_tab: 0,
            history_stack: Vec::new(),

            hwbp_addr_str: "".to_string(),
            hwbp_active: false,
            frozen_regs: None,
            status_msg: "GUI is ready.".to_string(),
            capstone: cs,
        }
    }

    fn process_net_events(&mut self) {
        while let Ok(msg) = self.ui_rx.try_recv() {
            match msg {
                UiNotify::Error(err) => self.status_msg = format!("Error: {}", err),
                UiNotify::BaseResult(base) => {
                    self.base_address = base;
                    self.status_msg = format!("Found base: 0x{:x}", base);
                }
                UiNotify::MemResult(addr, data) => {
                    self.status_msg = format!("Read {} bytes from 0x{:x}", data.len(), addr);
                    self.last_read_addr = addr;
                    self.raw_memory = data.clone();
                    self.disasm_text = disassemble_arm64(self.capstone.as_ref(), &data, addr);
                }
                UiNotify::HwbpSetResult(success) => {
                    self.hwbp_active = success;
                    self.status_msg = if success {
                        "HWBP Active".to_owned()
                    } else {
                        "HWBP Failed / Removed".to_owned()
                    };
                }
                UiNotify::HwbpTriggered(regs) => {
                    self.frozen_regs = Some(regs);
                    self.status_msg = "HWBP Triggered! THREAD FROZEN.".to_string();
                }
                UiNotify::HwbpResumed => {
                    self.frozen_regs = None;
                    self.status_msg = "Thread Resumed.".to_string();
                }
            }
        }
    }

    fn draw_target_panel(&mut self, ui: &mut egui::Ui) {
        ui.group(|ui| {
            ui.heading("🎯 Target Config");
            ui.horizontal(|ui| {
                ui.label("Module Name:");
                ui.text_edit_singleline(&mut self.target_module);
                if ui.button("Get Base").clicked() {
                    let tx = self.net_tx.clone();
                    let module = self.target_module.clone();
                    let notify = self.tx.clone(); // In StealthApp::new, we need to save `ui_tx` as `tx` for GUI loops
                    std::thread::spawn(move || {
                        let (cb_tx, cb_rx) = mpsc::channel();
                        if tx.send(NetRequest::GetBase(module, cb_tx)).is_ok() {
                            if let Ok(base) = cb_rx.recv() {
                                let _ = notify.send(UiNotify::BaseResult(base));
                            }
                        }
                    });
                }
            });
            if self.base_address > 0 {
                ui.label(
                    egui::RichText::new(format!("Base Address: 0x{:X}", self.base_address))
                        .color(egui::Color32::GREEN)
                        .strong(),
                );
            } else {
                ui.label(
                    egui::RichText::new("Base Address: Not Acquired")
                        .color(egui::Color32::DARK_GRAY),
                );
            }
        });
    }

    fn draw_memory_panel(&mut self, ui: &mut egui::Ui) {
        ui.group(|ui| {
            ui.heading("🔭 Memory Inspector");
            ui.horizontal(|ui| {
                if !self.history_stack.is_empty() {
                    if ui.button("⬅ Back").clicked() {
                        let prev_addr = self.history_stack.pop().unwrap();
                        self.read_addr_str = format!("0x{:x}", prev_addr);
                        let size = self.read_size_str.parse::<u32>().unwrap_or(128);
                        let tx = self.net_tx.clone();
                        let notify = self.tx.clone();
                        std::thread::spawn(move || {
                            let (cb_tx, cb_rx) = mpsc::channel();
                            if tx.send(NetRequest::ReadMem(prev_addr, size, cb_tx)).is_ok() {
                                if let Ok(data) = cb_rx.recv() { let _ = notify.send(UiNotify::MemResult(prev_addr, data)); }
                            }
                        });
                    }
                }

                ui.label("Address (HEX):");
                ui.text_edit_singleline(&mut self.read_addr_str);
                ui.label("Size:");
                ui.add(egui::TextEdit::singleline(&mut self.read_size_str).desired_width(50.0));

                if ui.button("READ").clicked() {
                    if let Ok(addr) = u64::from_str_radix(self.read_addr_str.trim_start_matches("0x"), 16) {
                        if let Ok(size) = self.read_size_str.parse::<u32>() {
                            self.history_stack.clear(); // Clear history on manual read
                            let tx = self.net_tx.clone();
                            let notify = self.tx.clone();
                            std::thread::spawn(move || {
                                let (cb, cb_rx) = mpsc::channel();
                                if tx.send(NetRequest::ReadMem(addr, size, cb)).is_ok() {
                                    if let Ok(data) = cb_rx.recv() { let _ = notify.send(UiNotify::MemResult(addr, data)); }
                                }
                            });
                        }
                    }
                }
            });

            ui.separator();
            ui.horizontal(|ui| {
                ui.selectable_value(&mut self.memory_tab, 0, "💻 Hex & Pointers Data");
                ui.selectable_value(&mut self.memory_tab, 1, "⚙ Disassembler (Code)");

                ui.with_layout(egui::Layout::right_to_left(egui::Align::Center), |ui| {
                    if ui.button("📋 Copy All (Hex)").clicked() {
                        let mut dump = String::with_capacity(self.raw_memory.len() * 4 + 100);
                        for (i, chunk) in self.raw_memory.chunks(8).enumerate() {
                            dump.push_str(&format!("0x{:010X} | ", self.last_read_addr + (i * 8) as u64));
                            for b in chunk {
                                dump.push_str(&format!("{:02X} ", b));
                            }
                            dump.push('\n');
                        }
                        ui.output_mut(|o| o.copied_text = dump);
                    }
                });
            });
            ui.separator();

            if self.memory_tab == 0 {
                egui::ScrollArea::vertical().max_height(350.0).auto_shrink([false, false]).show(ui, |ui| {
                    if self.raw_memory.is_empty() {
                        ui.label(egui::RichText::new("No data loaded. Read an address first.").color(egui::Color32::DARK_GRAY));
                        return;
                    }

                    egui::Grid::new("hex_grid").striped(true).spacing([20.0, 6.0]).show(ui, |ui| {
                        ui.heading("Offset");
                        ui.heading("Hex Bytes (8 per row)");
                        ui.heading("Interpretation / Pointers");
                        ui.end_row();

                        let mut i = 0;
                        while i < self.raw_memory.len() {
                            let end = (i + 8).min(self.raw_memory.len());
                            let chunk = &self.raw_memory[i..end];

                            // 1. Current Address
                            let current_addr = self.last_read_addr + i as u64;
                            ui.label(egui::RichText::new(format!("0x{:010X}", current_addr)).color(egui::Color32::GRAY));

                            // 2. Hex String
                            let mut hex_str = String::new();
                            for b in chunk { hex_str.push_str(&format!("{:02X} ", b)); }
                            ui.label(egui::RichText::new(hex_str).monospace().color(egui::Color32::from_rgb(200, 200, 255)));
                                                // 3. Multi-format Decoder (u64, f32, Ptr)
                                if chunk.len() == 8 {
                                    let mut arr = [0u8; 8];
                                    arr.copy_from_slice(chunk);
                                    let val = u64::from_le_bytes(arr);

                                    // Extract lower 32-bits for traditional f32
                                    let f32_val = f32::from_bits(u32::from_le_bytes(arr[0..4].try_into().unwrap()));
                                    // Extract full f64
                                    let _f64_val = f64::from_bits(val);

                                    ui.horizontal(|ui| {
                                        // Standard Integer
                                        ui.label(egui::RichText::new(format!("u64: {}", val)).color(egui::Color32::DARK_GRAY));

                                        // Float Check (filter out NaNs/Infinities if possible, or just print)
                                        if f32_val.is_finite() && f32_val.abs() >= 0.0001 && f32_val.abs() < 1000000.0 {
                                            ui.label(egui::RichText::new(format!(" | f32: {:.4}", f32_val)).color(egui::Color32::LIGHT_BLUE));
                                        } else if val == 0 {
                                            ui.label(egui::RichText::new(" | f32: 0.0").color(egui::Color32::DARK_GRAY));
                                        }

                                        // Pointer Action: Allow clicking on almost anything > 1MB just in case it's a `.text` allocation or heap
                                        if val > 0x100000 && val < 0x0000FFFFFFFFFFFF {
                                            ui.add_space(5.0);
                                            let link_resp = ui.link(egui::RichText::new(format!("⮞ 0x{:X}", val)).color(egui::Color32::from_rgb(255, 215, 0)));

                                            // Handle interaction
                                            if link_resp.clicked() {
                                                self.history_stack.push(self.last_read_addr);
                                                self.read_addr_str = format!("0x{:x}", val);
                                                self.memory_tab = 0; // Stay in Hex Mode
                                                let size = self.read_size_str.parse::<u32>().unwrap_or(128);
                                                let tx = self.net_tx.clone();
                                                let notify = self.tx.clone();
                                                std::thread::spawn(move || {
                                                    let (cb, cb_rx) = mpsc::channel();
                                                    let _ = tx.send(NetRequest::ReadMem(val, size, cb));
                                                    if let Ok(data) = cb_rx.recv() { let _ = notify.send(UiNotify::MemResult(val, data)); }
                                                });
                                            }

                                            // [ASM] Button
                                            if ui.link(egui::RichText::new("[ASM]").color(egui::Color32::from_rgb(100, 255, 100))).on_hover_text("Dump 1024 bytes and switch to Disassembler").clicked()
                                               || link_resp.double_clicked() {
                                                self.history_stack.push(self.last_read_addr);
                                                self.read_addr_str = format!("0x{:x}", val);
                                                self.read_size_str = "1024".to_string();
                                                self.memory_tab = 1;
                                                let tx = self.net_tx.clone();
                                                let notify = self.tx.clone();
                                                std::thread::spawn(move || {
                                                    let (cb, cb_rx) = mpsc::channel();
                                                    let _ = tx.send(NetRequest::ReadMem(val, 1024, cb));
                                                    if let Ok(data) = cb_rx.recv() { let _ = notify.send(UiNotify::MemResult(val, data)); }
                                                });
                                            }
                                        }
                                    });
                                } else {
                                ui.label(egui::RichText::new("Incomplete block").color(egui::Color32::DARK_GRAY));
                            }

                            ui.end_row();
                            i += 8;
                        }
                    });
                });
            } else {
                egui::ScrollArea::vertical().max_height(350.0).auto_shrink([false, false]).show(ui, |ui| {
                    ui.label(egui::RichText::new(&self.disasm_text).monospace().color(egui::Color32::LIGHT_GREEN));
                });
            }
        });
    }

    fn draw_debugger_panel(&mut self, ui: &mut egui::Ui) {
        ui.group(|ui| {
            ui.heading("⚡ Ring-0 HWBP Debugger");
            ui.horizontal(|ui| {
                ui.label("Trigger Addr (HEX):");
                ui.text_edit_singleline(&mut self.hwbp_addr_str);

                let btn_text = if self.hwbp_active {
                    "Remove HWBP"
                } else {
                    "Set HWBP"
                };
                if ui.button(btn_text).clicked() {
                    let tx = self.net_tx.clone();
                    let notify = self.tx.clone();
                    if self.hwbp_active {
                        std::thread::spawn(move || {
                            let (cb, rx) = mpsc::channel();
                            let _ = tx.send(NetRequest::RemoveHwbp(cb));
                            if let Ok(_) = rx.recv() {
                                let _ = notify.send(UiNotify::HwbpSetResult(false));
                            }
                        });
                    } else if let Ok(addr) =
                        u64::from_str_radix(self.hwbp_addr_str.trim_start_matches("0x"), 16)
                    {
                        std::thread::spawn(move || {
                            let (cb, rx) = mpsc::channel();
                            let _ = tx.send(NetRequest::SetHwbp(addr, cb));
                            if let Ok(act) = rx.recv() {
                                let _ = notify.send(UiNotify::HwbpSetResult(act));
                            }
                        });
                    }
                }
            });

            if let Some(mut regs) = self.frozen_regs {
                ui.separator();
                ui.heading(
                    egui::RichText::new("🚨 THREAD FROZEN IN KERNEL SPACE 🚨")
                        .color(egui::Color32::RED),
                );

                egui::Grid::new("regs_grid")
                    .num_columns(4)
                    .spacing([15.0, 5.0])
                    .show(ui, |ui| {
                        ui.label(
                            egui::RichText::new(format!("PC: 0x{:X}", regs.pc))
                                .strong()
                                .color(egui::Color32::YELLOW),
                        );
                        ui.label(format!("SP: 0x{:X}", regs.sp));
                        ui.label(format!("X30 (LR): 0x{:X}", regs.regs[30]));
                        ui.end_row();
                    });

                ui.separator();
                let mut x0_hex = format!("{:x}", regs.regs[0]);
                ui.horizontal(|ui| {
                    ui.label("Edit X0 (HEX) Register:");
                    if ui.text_edit_singleline(&mut x0_hex).changed() {
                        if let Ok(val) = u64::from_str_radix(&x0_hex, 16) {
                            regs.regs[0] = val;
                            self.frozen_regs = Some(regs);
                        }
                    }
                });

                ui.horizontal(|ui| {
                    if ui.button("▶ Resume (Normal)").clicked() {
                        let tx = self.net_tx.clone();
                        let notify = self.tx.clone();
                        std::thread::spawn(move || {
                            let (cb, rx) = mpsc::channel();
                            let _ = tx.send(NetRequest::ResumeHwbp(regs, cb));
                            if let Ok(_) = rx.recv() {
                                let _ = notify.send(UiNotify::HwbpResumed);
                            }
                        });
                    }
                    if ui
                        .button("⏭ Bypass (Early Return)")
                        .on_hover_text(
                            "Sets PC = X30 (Link Register) to exit function immediately.",
                        )
                        .clicked()
                    {
                        regs.pc = regs.regs[30];
                        regs.pstate &= !(1u64 << 21); // Clear Single Step flag
                        let tx = self.net_tx.clone();
                        let notify = self.tx.clone();
                        std::thread::spawn(move || {
                            let (cb, rx) = mpsc::channel();
                            let _ = tx.send(NetRequest::ResumeHwbp(regs, cb));
                            if let Ok(_) = rx.recv() {
                                let _ = notify.send(UiNotify::HwbpResumed);
                            }
                        });
                    }
                    if ui.button("⏭ Skip Instruction (PC += 4)").clicked() {
                        regs.pc += 4;
                        regs.pstate &= !(1u64 << 21);
                        let tx = self.net_tx.clone();
                        let notify = self.tx.clone();
                        std::thread::spawn(move || {
                            let (cb, rx) = mpsc::channel();
                            let _ = tx.send(NetRequest::ResumeHwbp(regs, cb));
                            if let Ok(_) = rx.recv() {
                                let _ = notify.send(UiNotify::HwbpResumed);
                            }
                        });
                    }
                });
            } else {
                ui.label(
                    egui::RichText::new("Thread is currently running naturally.")
                        .color(egui::Color32::DARK_GRAY),
                );
            }
        });
    }
}

impl eframe::App for StealthApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        self.process_net_events();

        egui::TopBottomPanel::bottom("status").show(ctx, |ui| {
            ui.label(egui::RichText::new(&self.status_msg).strong());
        });

        egui::CentralPanel::default().show(ctx, |ui| {
            egui::ScrollArea::vertical().show(ui, |ui| {
                self.draw_target_panel(ui);
                ui.add_space(5.0);
                self.draw_memory_panel(ui);
                ui.add_space(5.0);
                self.draw_debugger_panel(ui);
            });
        });

        // Fast UI polling rate for networking smoothness
        ctx.request_repaint_after(Duration::from_millis(50));
    }
}

fn disassemble_arm64(capstone: Option<&capstone::Capstone>, data: &[u8], base_addr: u64) -> String {
    match capstone {
        Some(capstone) => {
            if let Ok(insns) = capstone.disasm_all(data, base_addr) {
                if insns.is_empty() {
                    return "No valid instructions found.\n(You are likely reading a DATA section or padding, not CODE!)".to_string();
                }
                let mut out = String::new();
                for ins in insns.as_ref() {
                    out.push_str(&format!(
                        "0x{:X}: {} {}\n",
                        ins.address(),
                        ins.mnemonic().unwrap_or(""),
                        ins.op_str().unwrap_or("")
                    ));
                }
                out
            } else {
                "Failed to disassemble".to_string()
            }
        }
        None => "Capstone initialization failed".to_string(),
    }
}

// ----------------- NETWORK LAYER -----------------

fn net_send(stream: &mut TcpStream, cmd: &NetCommand, payload: &[u8]) -> Result<(), String> {
    stream
        .write_all(bytemuck::bytes_of(cmd))
        .map_err(|e| e.to_string())?;
    if !payload.is_empty() {
        stream.write_all(payload).map_err(|e| e.to_string())?;
    }
    Ok(())
}

fn net_recv(stream: &mut TcpStream) -> Result<(NetCommand, Vec<u8>), String> {
    let mut header = [0u8; std::mem::size_of::<NetCommand>()];
    stream.read_exact(&mut header).map_err(|e| e.to_string())?;

    let cmd: NetCommand = *bytemuck::from_bytes(&header);
    let mut payload = vec![0u8; cmd.size as usize];
    if cmd.size > 0 {
        stream.read_exact(&mut payload).map_err(|e| e.to_string())?;
    }
    Ok((cmd, payload))
}

pub fn network_worker(rx: mpsc::Receiver<NetRequest>, ui_notify: mpsc::Sender<UiNotify>) {
    let mut stream = match TcpStream::connect("127.0.0.1:1337") {
        Ok(s) => s,
        Err(e) => {
            let _ = ui_notify.send(UiNotify::Error(format!("TCP Connect failed: {}", e)));
            return;
        }
    };

    let mut last_poll = Instant::now();
    let mut hwbp_active = false;

    // Shared register cache for GDB
    let mut last_frozen_regs: Option<StealthPtRegs> = None;

    loop {
        // Actor requests
        while let Ok(msg) = rx.try_recv() {
            match msg {
                NetRequest::GetBase(name, cb) => {
                    let req = NetCommand {
                        cmd_type: CMD_GET_BASE,
                        address: 0,
                        size: name.len() as u32,
                    };
                    if net_send(&mut stream, &req, name.as_bytes()).is_ok() {
                        if let Ok((res, _)) = net_recv(&mut stream) {
                            let _ = cb.send(res.address);
                        }
                    }
                }
                NetRequest::ReadMem(addr, size, cb) => {
                    let req = NetCommand {
                        cmd_type: CMD_READ_MEM,
                        address: addr,
                        size,
                    };
                    if net_send(&mut stream, &req, &[]).is_ok() {
                        if let Ok((_, data)) = net_recv(&mut stream) {
                            let _ = cb.send(data);
                        }
                    }
                }
                NetRequest::SetHwbp(addr, cb) => {
                    let req = NetCommand {
                        cmd_type: CMD_SET_HWBP,
                        address: addr,
                        size: 0,
                    };
                    if net_send(&mut stream, &req, &[]).is_ok() {
                        if let Ok((res, _)) = net_recv(&mut stream) {
                            hwbp_active = res.address == 1;
                            let _ = cb.send(hwbp_active);
                        }
                    }
                }
                NetRequest::RemoveHwbp(cb) => {
                    let req = NetCommand {
                        cmd_type: CMD_REMOVE_HWBP,
                        address: 0,
                        size: 0,
                    };
                    if net_send(&mut stream, &req, &[]).is_ok() {
                        if let Ok((_res, _)) = net_recv(&mut stream) {
                            hwbp_active = false;
                            last_frozen_regs = None;
                            let _ = cb.send(true);
                        }
                    }
                }
                NetRequest::ResumeHwbp(regs, cb) => {
                    let payload = bytemuck::bytes_of(&regs);
                    let req = NetCommand {
                        cmd_type: CMD_RESUME_HWBP,
                        address: 0,
                        size: payload.len() as u32,
                    };
                    if net_send(&mut stream, &req, payload).is_ok() {
                        if net_recv(&mut stream).is_ok() {
                            last_frozen_regs = None;
                            let _ = cb.send(());
                        }
                    }
                }
                NetRequest::GdbGetRegisters(cb) => {
                    let _ = cb.send(last_frozen_regs);
                }
            }
        }

        // Background HWBP Polling
        if hwbp_active && last_poll.elapsed() >= Duration::from_millis(500) {
            let req = NetCommand {
                cmd_type: CMD_CHECK_DEBUG_EVENT,
                address: 0,
                size: 0,
            };
            if net_send(&mut stream, &req, &[]).is_ok() {
                if let Ok((res, data)) = net_recv(&mut stream) {
                    if res.size > 0 && data.len() == std::mem::size_of::<StealthPtRegs>() {
                        let regs: StealthPtRegs = *bytemuck::from_bytes(&data);
                        last_frozen_regs = Some(regs);
                        let _ = ui_notify.send(UiNotify::HwbpTriggered(regs));
                    }
                }
            }
            last_poll = Instant::now();
        }
        std::thread::sleep(Duration::from_millis(10));
    }
}

fn main() -> eframe::Result<()> {
    let options = eframe::NativeOptions {
        viewport: egui::ViewportBuilder::default()
            .with_inner_size([750.0, 650.0])
            .with_title("Stealth LKM Analyzer"),
        ..Default::default()
    };
    eframe::run_native(
        "Stealth LKM Analyzer",
        options,
        Box::new(|cc| Box::new(StealthApp::new(cc))),
    )
}
