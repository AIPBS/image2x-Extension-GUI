// Copyright (C) 2026 AIPEAC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

use std::fs;
use std::io::{BufRead, BufReader, BufWriter, Write};
use std::os::unix::fs::PermissionsExt;
use std::os::unix::net::{UnixListener, UnixStream};
use std::path::PathBuf;
use std::thread;

use anyhow::Result;
use clap::Parser;

#[derive(Debug, Parser)]
#[command(name = "image2x-core", version, about = "image2x Linux business core")]
struct Arguments {
    #[arg(long)]
    socket: PathBuf,
    #[arg(long, default_value = ".")]
    application_directory: PathBuf,
}

fn main() -> Result<()> {
    let arguments = Arguments::parse();
    if let Some(parent) = arguments.socket.parent() {
        fs::create_dir_all(parent)?;
    }
    if arguments.socket.exists() {
        fs::remove_file(&arguments.socket)?;
    }
    let listener = UnixListener::bind(&arguments.socket)?;
    fs::set_permissions(&arguments.socket, fs::Permissions::from_mode(0o600))?;
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                let application_directory = arguments.application_directory.clone();
                thread::spawn(move || handle_connection(stream, &application_directory));
            }
            Err(error) => eprintln!("image2x-core: socket accept failed: {error}"),
        }
    }
    Ok(())
}

fn handle_connection(stream: UnixStream, application_directory: &PathBuf) {
    let reader = BufReader::new(match stream.try_clone() {
        Ok(stream) => stream,
        Err(error) => {
            eprintln!("image2x-core: socket clone failed: {error}");
            return;
        }
    });
    let mut writer = BufWriter::new(stream);
    for line in reader.lines() {
        let line = match line {
            Ok(line) => line,
            Err(error) => {
                eprintln!("image2x-core: socket read failed: {error}");
                return;
            }
        };
        let response = match image2x_core::handle_line(&line, application_directory) {
            Ok(response) => response,
            Err(error) => image2x_core::error_line(&line, &error),
        };
        if writer.write_all(response.as_bytes()).is_err() || writer.flush().is_err() {
            return;
        }
    }
}
