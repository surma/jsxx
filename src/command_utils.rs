use std::io::{Read, Result, Write};
use std::process::{Command, ExitStatus, Stdio};

pub fn pipe_through_shell<T: AsRef<std::ffi::OsStr>>(
    command: &str,
    args: &[T],
    input: &[u8],
) -> Result<(ExitStatus, Vec<u8>, Vec<u8>)> {
    let mut child = Command::new(command)
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .args(args)
        .spawn()
        .expect(format!("Couldn’t spawn {}", command).as_str());
    child
        .stdin
        .as_ref()
        .expect(format!("Couldn’t claim {}’s stdin", command).as_str())
        .write_all(input)
        .expect(format!("Couldn’t write to {}’s stdin", command).as_str());
    let exit_status = child.wait()?;
    let mut stdout: Vec<u8> = vec![];
    child.stdout.take().unwrap().read_to_end(&mut stdout)?;
    let mut stderr: Vec<u8> = vec![];
    child.stderr.take().unwrap().read_to_end(&mut stderr)?;
    Ok((exit_status, stdout, stderr))
}
