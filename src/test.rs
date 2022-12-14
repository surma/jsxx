use crate::transpiler::Transpiler;

use super::*;
use anyhow::Result;
use uuid::Uuid;

#[test]
fn increment_postfix() -> Result<()> {
    let output = compile_and_run(
        r#"
            let a = 1;
            let b = a++;
            IO.write_to_stdout((a+b) < 4 ? "y" : "n");
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

#[test]
fn increment_prefix() -> Result<()> {
    let output = compile_and_run(
        r#"
            let a = 1;
            let b = ++a;
            IO.write_to_stdout((a+b) >= 4 ? "y" : "n");
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

#[test]
fn basic_program() -> Result<()> {
    let output = compile_and_run(
        r#"
            IO.write_to_stdout("hello");
        "#,
    )?;
    assert_eq!(output, "hello");
    Ok(())
}

#[test]
fn variable() -> Result<()> {
    let output = compile_and_run(
        r#"
            let a = "hello";
            IO.write_to_stdout(a);
        "#,
    )?;
    assert_eq!(output, "hello");
    Ok(())
}

#[test]
fn copy_behavior_string() -> Result<()> {
    let output = compile_and_run(
        r#"
            let a = "hello";
            let b = a;
            b = b + "!";
            IO.write_to_stdout(a + b);
        "#,
    )?;
    assert_eq!(output, "hellohello!");
    Ok(())
}

#[test]
fn copy_behavior_number() -> Result<()> {
    let output = compile_and_run(
        r#"
            let a = 1.0;
            let b = a;
            b = 3;
            IO.write_to_stdout(((a + b) == 4) ? "y" : "n");
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

#[test]
fn copy_behavior_functions() -> Result<()> {
    let output = compile_and_run(
        r#"
            let a = "x";
            function f(v) {
                v = v + "!";
            }
            f(a);
            IO.write_to_stdout(a);
        "#,
    )?;
    assert_eq!(output, "x");
    Ok(())
}

#[test]
fn variable_assign() -> Result<()> {
    let output = compile_and_run(
        r#"
            let a = "hi";
            a = "hello";
            IO.write_to_stdout(a);
        "#,
    )?;
    assert_eq!(output, "hello");
    Ok(())
}

#[test]
fn ternary() -> Result<()> {
    let output = compile_and_run(
        r#"
            IO.write_to_stdout(2 == 3 ? "yes" : "no");
        "#,
    )?;
    assert_eq!(output, "no");
    Ok(())
}

#[test]
fn compare() -> Result<()> {
    let output = compile_and_run(
        r#"
            let b = (2 == 2) && (3 != 4) && (1 < 2) && (2<=2) && (3>=3) && (4 > 3);
            IO.write_to_stdout(b ? "yes" : "no");
        "#,
    )?;
    assert_eq!(output, "yes");
    Ok(())
}

#[test]
fn arrow_func() -> Result<()> {
    let output = compile_and_run(
        r#"
            IO.write_to_stdout("" + (() => "test")());
        "#,
    )?;
    assert!(output.starts_with("test"));
    Ok(())
}

#[test]
fn arrow_func_with_body() -> Result<()> {
    let output = compile_and_run(
        r#"
            IO.write_to_stdout("" + (() => { 1 + 1; return "test";})());
        "#,
    )?;
    assert!(output.starts_with("test"));
    Ok(())
}

#[test]
fn closure_simple() -> Result<()> {
    let output = compile_and_run(
        r#"
            let x = "wrong";
            function a() {
                x = "hi";
            }

            a();
            IO.write_to_stdout(x);
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn closure_obj() -> Result<()> {
    let output = compile_and_run(
        r#"
            let x = {value: "wrong"};
            function a() {
                x.value = "hi";
            }

            a();
            IO.write_to_stdout(x.value);
        "#,
    )?;
    assert!(output.starts_with("hi"));
    Ok(())
}

#[test]
fn func_decl() -> Result<()> {
    let output = compile_and_run(
        r#"
            function a() {
                return "test";
            }

            IO.write_to_stdout("" + a());
        "#,
    )?;
    assert!(output.starts_with("test"));
    Ok(())
}

#[test]
fn full_func() -> Result<()> {
    let output = compile_and_run(
        r#"
            IO.write_to_stdout("" + (function () { return "test";})());
        "#,
    )?;
    assert!(output.starts_with("test"));
    Ok(())
}

#[test]
fn if_else() -> Result<()> {
    let output = compile_and_run(
        r#"
            let a;
            if(1 == 1) {
                a = "y";
            } else {
                a = "n";
            }
            let b;
            if(1 == 2) {
                b = "y";
            } else {
                b = "n";
            }
            IO.write_to_stdout(a + b);
        "#,
    )?;
    assert_eq!(output, "yn");
    Ok(())
}
#[test]
fn number_coalesc() -> Result<()> {
    let output = compile_and_run(
        r#"
            IO.write_to_stdout("" + 123);
        "#,
    )?;
    assert!(output.starts_with("123."));
    Ok(())
}

#[test]
fn array_literals() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = ["a", "b", "c"]
            IO.write_to_stdout(v.join(","));
        "#,
    )?;
    assert_eq!(output, "a,b,c");
    Ok(())
}

#[test]
fn array_reference() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = ["a", "b", "c"]
            let x = v;
            x.push("d");
            IO.write_to_stdout(v.join(","));
        "#,
    )?;
    assert_eq!(output, "a,b,c,d");
    Ok(())
}

#[test]
fn array_access() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = ["a", "b"];
            IO.write_to_stdout(v[0] + v[1]);
        "#,
    )?;
    assert_eq!(output, "ab");
    Ok(())
}

#[test]
fn array_push() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = ["a", "b"];
            v.push("c");
            IO.write_to_stdout(v.join(","));
        "#,
    )?;
    assert_eq!(output, "a,b,c");
    Ok(())
}

#[test]
fn array_map() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = ["a", "b", "c"];
            IO.write_to_stdout(v.map(v => v + "!").join(","));
        "#,
    )?;
    assert_eq!(output, "a!,b!,c!");
    Ok(())
}

#[test]
fn array_filter() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = [1, 2, 3, 4, 5];
            IO.write_to_stdout(v.filter(v => v % 2 == 0).length == 2 ? "yes" : "no");
        "#,
    )?;
    assert_eq!(output, "yes");
    Ok(())
}

#[test]
fn array_reduce() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = ["a", "b", "c"].reduce((acc, c) => acc + c, "X");
            let v2 = ["a", "b", "c"].reduce((acc, c) => acc + c);
            IO.write_to_stdout(v + v2);
        "#,
    )?;
    assert_eq!(output, "Xabcabc");
    Ok(())
}

#[test]
fn array_set_length() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = ["a", "b", "c"];
            v.length = 2;
            IO.write_to_stdout(v.join(","));
        "#,
    )?;
    assert_eq!(output, "a,b");
    Ok(())
}

#[test]
fn array_length() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = ["a", "b", "c"];
            IO.write_to_stdout(v.length > 2 ? "yes" : "no");
        "#,
    )?;
    assert_eq!(output, "yes");
    Ok(())
}

#[test]
fn object_lit() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = {a: "v"};
            IO.write_to_stdout(v.a);
        "#,
    )?;
    assert_eq!(output, "v");
    Ok(())
}

#[test]
fn object_func() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = {a: () => "hi"};
            IO.write_to_stdout(v.a());
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn object_func_prop() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = {a() { return "hi"; }};
            IO.write_to_stdout(v.a());
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn object_func_this() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = {marker: "flag", a: function() { return this.marker; }};
            IO.write_to_stdout(v.a());
        "#,
    )?;
    assert_eq!(output, "flag");
    Ok(())
}

#[test]
fn object_assign() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = {marker: "flag"};
            v.marker = "hi";
            IO.write_to_stdout(v.marker);
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn object_assign2() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = {};
            v["marker"] = "hi";
            IO.write_to_stdout(v.marker);
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn object_shorthand() -> Result<()> {
    let output = compile_and_run(
        r#"
            let a = "hi";
            let v = {a};
            IO.write_to_stdout(v.a);
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn object_getter() -> Result<()> {
    let output = compile_and_run(
        r#"
            let state = "hi";
            let v = {
                get prop() {
                    return state;
                },
            };
            IO.write_to_stdout(v.prop);
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn object_setter() -> Result<()> {
    let output = compile_and_run(
        r#"
            let state = {v: "test"};
            let v = {
                set prop(v) {
                    state.v = v;
                },
            };
            v.prop = "hi";
            IO.write_to_stdout(state.v);
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn object_getter_this() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = {
                state: "hi",
                get prop() {
                    return this.state;
                },
            };
            IO.write_to_stdout(v.prop);
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn object_equality() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = {};
            let v1 = v;
            let v2 = {};
            let c = 0;
            if(v == v1) {
                c = c+1;
            }
            if(v != v2) {
                c = c+2;
            }

            IO.write_to_stdout("" + c);
        "#,
    )?;
    assert_eq!(&output[0..2], "3.");
    Ok(())
}

#[test]
fn object_computed() -> Result<()> {
    let output = compile_and_run(
        r#"
            let k = "abc";
            let v = {
                [k]: "hi",
                ["x"+"y"]: "hi2"
            };
            IO.write_to_stdout(v.abc + v.xy);
        "#,
    )?;
    assert_eq!(output, "hihi2");
    Ok(())
}

#[test]
fn json_stringify_array() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = {x: []};
            IO.write_to_stdout(JSON.stringify(v));
        "#,
    )?;
    assert_eq!(output, r#"{"x":[]}"#);
    Ok(())
}

#[test]
fn json_parse_string_escapes() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = JSON.parse("\"x\n\"");
            IO.write_to_stdout(v);
        "#,
    )?;
    assert_eq!(output, "x\n");
    Ok(())
}

#[test]
fn for_loop() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = [];
            for(let i = 0; i < 4; i++) {
                v.push(i)
            }
            IO.write_to_stdout(v.length == 4 ? "y" : "n");
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

#[test]
fn while_loop() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = [];
            let i = 0;
            while(i < 4) {
                v.push("a")
                i = i + 1;
            }
            IO.write_to_stdout(v.join(""));
        "#,
    )?;
    assert_eq!(output, "aaaa");
    Ok(())
}

#[test]
fn while_loop_break() -> Result<()> {
    let output = compile_and_run(
        r#"
            let v = [];
            let i = 0;
            while(true) {
                v.push("a")
                i = i + 1;
                if(i >= 4) break;
            }
            IO.write_to_stdout(v.join(""));
        "#,
    )?;
    assert_eq!(output, "aaaa");
    Ok(())
}

#[test]
fn iterator() -> Result<()> {
    let output = compile_and_run(
        r#"
            let it = {
                i: 0,
                next() {
                    if(this.i > 4) {
                        return {done: true};
                    }
                    return {
                        value: this.i++,
                        done: false
                    };
                },
                [Symbol.iterator]() {
                    return this;
                }
            };
            let arr = [];
            for(let v of it) {
                arr.push(v)
            }
            let sum = arr.reduce((sum, c) => sum +c, 0);
            IO.write_to_stdout(sum == 10 ? "y" : "n");
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

#[test]
fn iterator_array() -> Result<()> {
    let output = compile_and_run(
        r#"
            let arr = [];
            for(let v of [1,2,3,4]) {
                arr.push(v)
            }
            let sum = arr.reduce((sum, c) => sum +c, 0);
            IO.write_to_stdout(sum == 10 ? "y" : "n");
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

#[test]
fn generator_iterator_protocol() -> Result<()> {
    let output = compile_and_run(
        r#"
            function* gen() {
                yield "hi";
                return;
            }
            let it = gen();
            IO.write_to_stdout(it.next().value);
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn generator_iterator_protocol_indirect() -> Result<()> {
    let output = compile_and_run(
        r#"
            function* gen() {
                yield "hi";
                return;
            }
            let it = gen()[Symbol.iterator]();
            IO.write_to_stdout(it.next().value);
        "#,
    )?;
    assert_eq!(output, "hi");
    Ok(())
}

#[test]
fn generator() -> Result<()> {
    let output = compile_and_run(
        r#"
            function* gen() {
                yield 1;
                yield 2;
                yield 3;
                yield 4;
                return;
            }
            let arr = [];
            for(let v of gen()) {
                arr.push(v)
            }
            let sum = arr.reduce((sum, c) => sum +c, 0);
            IO.write_to_stdout(sum == 10 ? "y" : "n");
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

#[test]
fn generator_delegate_builtin() -> Result<()> {
    let output = compile_and_run(
        r#"
            function* gen() {
                yield* [1, 2];
                yield* [3, 4];
            }
            let arr = [];
            for(let v of gen()) {
                arr.push(v)
            }
            let sum = arr.reduce((sum, c) => sum +c, 0);
            IO.write_to_stdout(sum == 10 ? "y" : "n");
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

#[test]
fn generator_delegate() -> Result<()> {
    let output = compile_and_run(
        r#"
            function* gen2() {
                yield 1;
                yield 2;
                yield 3;
                yield 4;
                return;
            }
            function* gen() {
                yield* gen2();
            }
            let arr = [];
            for(let v of gen()) {
                arr.push(v)
            }
            let sum = arr.reduce((sum, c) => sum +c, 0);
            IO.write_to_stdout(sum == 10 ? "y" : "n");
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

#[test]
fn generator_exception() -> Result<()> {
    let output = compile_and_run(
        r#"
            function* gen() {
                throw "omg";
            }
            try {
                for(let v of gen()) {}
            } catch(e) {
                IO.write_to_stdout(e);
            }
        "#,
    )?;
    assert_eq!(output, "omg");
    Ok(())
}

#[test]
fn exceptions() -> Result<()> {
    let output = compile_and_run(
        r#"
            try {
                throw "y";
            } catch(e) {
                IO.write_to_stdout(e);
            }
        "#,
    )?;
    assert_eq!(output, "y");
    Ok(())
}

fn compile_and_run<T: AsRef<str>>(code: T) -> Result<String> {
    let name = Uuid::new_v4().to_string();
    let transpiler = Transpiler::new();
    let cpp = js_to_cpp(transpiler, code)?;
    cpp_to_binary(
        cpp,
        name.clone(),
        "clang++".to_string(),
        &Vec::<String>::new(),
    )?;
    let child = Command::new(format!("./{}", &name))
        .stdout(Stdio::piped())
        .spawn()?;
    let output = child.wait_with_output()?;
    std::fs::remove_file(&name)?;
    if output.stderr.len() > 0 {
        return Err(anyhow!(
            "Program printed to stderr: {}",
            String::from_utf8(output.stderr)?
        ));
    }
    Ok(String::from_utf8(output.stdout)?)
}
