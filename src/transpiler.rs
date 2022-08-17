use std::collections::HashSet;

use anyhow::{anyhow, Result};
use swc_ecma_ast::*;

pub struct Transpiler {
    pub globals: Vec<crate::globals::Global>,
    is_lhs: bool,
    is_generator: bool,
}

impl Transpiler {
    pub fn new() -> Transpiler {
        Transpiler {
            globals: vec![],
            is_lhs: false,
            is_generator: false,
        }
    }

    pub fn transpile_module(&mut self, module: &Module) -> Result<String> {
        let additional_headers: HashSet<String> = self
            .globals
            .iter()
            .flat_map(|global| {
                global
                    .additional_headers
                    .clone()
                    .unwrap_or(vec![])
                    .into_iter()
            })
            .collect();
        let additional_includes: String = additional_headers
            .into_iter()
            .map(|include| format!(r#"#include "{}""#, include))
            .collect::<Vec<String>>()
            .join("\n");

        let inits = self
            .globals
            .iter()
            .map(|global| global.init.clone().unwrap_or("".into()))
            .collect::<Vec<String>>()
            .join("\n");
        let global_exprs = self
            .globals
            .iter()
            .map(|global| format!("auto {} = {};", global.name, global.factory))
            .collect::<Vec<String>>()
            .join("\n");

        let transpiled_items: Vec<Result<String>> = module
            .body
            .iter()
            .map(|item| -> Result<String> {
                match item {
                    ModuleItem::ModuleDecl(_decl) => {
                        return Err(anyhow!("Module imports/exports are not support"))
                    }
                    ModuleItem::Stmt(stmt) => self.transpile_stmt(stmt),
                }
            })
            .collect();
        Ok(format!(
            r#"
                {additional_includes}
                #include <experimental/coroutine>
                #include "runtime/js_value.hpp"

                int main() {{
                    {inits}
                    {global_exprs}
                    {program}
                    return 0;
                }}
            "#,
            additional_includes = additional_includes,
            inits = inits,
            global_exprs = global_exprs,
            program = Result::<Vec<String>>::from_iter(transpiled_items)?.join(";\n")
        ))
    }

    fn transpile_stmt(&mut self, stmt: &Stmt) -> Result<String> {
        let transpiled_stmt = match stmt {
            Stmt::Decl(decl) => self.transpile_decl(decl)?,
            Stmt::Expr(expr_stmt) => self.transpile_expr(&expr_stmt.expr)?,
            Stmt::Block(block_stmt) => self.transpile_block_stmt(block_stmt)?,
            Stmt::Return(return_stmt) => self.transpile_return_stmt(return_stmt)?,
            Stmt::If(if_stmt) => self.transpile_if_stmt(if_stmt)?,
            Stmt::For(for_stmt) => self.transpile_for_stmt(for_stmt)?,
            Stmt::ForOf(for_of_stmt) => self.transpile_for_of_stmt(for_of_stmt)?,
            Stmt::While(while_stmt) => self.transpile_while_stmt(while_stmt)?,
            Stmt::Break(break_stmt) => self.transpile_break_stmt(break_stmt)?,
            _ => return Err(anyhow!("Unsupported statemt: {:?}", stmt)),
        };
        Ok(format!("{};", transpiled_stmt))
    }

    fn transpile_for_of_stmt(&mut self, for_of_stmt: &ForOfStmt) -> Result<String> {
        let left = match &for_of_stmt.left {
            VarDeclOrPat::VarDecl(var_decl) => self.transpile_var_decl(&var_decl)?,
            _ => return Err(anyhow!("Only simple variables are supported in for-of")),
        };

        let right = self.transpile_expr(&for_of_stmt.right)?;
        let body = self.transpile_stmt(&for_of_stmt.body)?;

        Ok(format!(
            r#"
                for({left} : {right}) {{
                    {body}
                }}
            "#,
            left = left,
            right = right,
            body = body,
        ))
    }

    fn transpile_break_stmt(&mut self, _break_stmt: &BreakStmt) -> Result<String> {
        Ok(format!("break"))
    }

    fn transpile_while_stmt(&mut self, while_stmt: &WhileStmt) -> Result<String> {
        let test = self.transpile_expr(&while_stmt.test)?;
        let body = self.transpile_stmt(&while_stmt.body)?;
        Ok(format!("while(({}).coerce_to_bool()) {{ {} }}", test, body))
    }

    fn transpile_for_stmt(&mut self, for_stmt: &ForStmt) -> Result<String> {
        let init = for_stmt
            .init
            .as_ref()
            .map(|var_decl_or_expr| match var_decl_or_expr {
                VarDeclOrExpr::Expr(expr) => self.transpile_expr(expr),
                VarDeclOrExpr::VarDecl(var_decl) => self.transpile_var_decl(var_decl),
            })
            .transpose()?
            .unwrap_or("".to_string());

        let test = for_stmt
            .test
            .as_ref()
            .map(|expr| self.transpile_expr(expr))
            .transpose()?
            .unwrap_or("".to_string());

        let update = for_stmt
            .update
            .as_ref()
            .map(|expr| self.transpile_expr(expr))
            .transpose()?
            .unwrap_or("".to_string());

        let body = self.transpile_stmt(for_stmt.body.as_ref())?;

        Ok(format!(
            r#"
                for({init};({test}).coerce_to_bool();{update}) {{
                    {body}
                }}
            "#,
            init = init,
            test = test,
            update = update,
            body = body,
        ))
    }

    fn transpile_if_stmt(&mut self, if_stmt: &IfStmt) -> Result<String> {
        let test = self.transpile_expr(&if_stmt.test)?;
        let cons = self.transpile_stmt(&if_stmt.cons)?;
        let alt = if_stmt
            .alt
            .as_ref()
            .map(|alt| self.transpile_stmt(alt.as_ref()))
            .transpose()?
            .unwrap_or("".into());
        Ok(format!(
            r#"
                if(({}).coerce_to_bool()) {{
                    {}
                }} else {{
                    {}
                }}
            "#,
            test, cons, alt
        ))
    }

    fn transpile_return_stmt(&mut self, return_stmt: &ReturnStmt) -> Result<String> {
        let arg = match &return_stmt.arg {
            None => "".to_string(),
            Some(expr) => self.transpile_expr(expr)?,
        };
        let ret_style = if self.is_generator {
            "co_return"
        } else {
            "return"
        };
        Ok(format!("{} {};", ret_style, arg))
    }

    fn transpile_decl(&mut self, decl: &Decl) -> Result<String> {
        match decl {
            Decl::Var(var_decl) => self.transpile_var_decl(var_decl),
            Decl::Fn(fn_decl) => self.transpile_fn_decl(fn_decl),
            _ => return Err(anyhow!("Unsupported declaration: {:?}", decl)),
        }
    }

    fn transpile_fn_decl(&mut self, fn_decl: &FnDecl) -> Result<String> {
        let name = format!("{}", fn_decl.ident.sym);
        let func = self.transpile_function(&fn_decl.function)?;
        Ok(format!("JSValue {} = {};", name, func))
    }

    fn transpile_var_decl(&mut self, var_decl: &VarDecl) -> Result<String> {
        if var_decl.kind != VarDeclKind::Let {
            return Err(anyhow!("Only `let` variable declarations are supported."));
        }
        if var_decl.decls.len() > 1 {
            return Err(anyhow!("Only single-variable let statements for now"));
        }
        self.transpile_var_declarator(&var_decl.decls[0])
    }

    fn transpile_var_declarator(&mut self, var_decl: &VarDeclarator) -> Result<String> {
        let ident = var_decl.name.as_ident().ok_or(anyhow!(
            "Only straight-up identifiers are supported for variable declarations for now."
        ))?;
        let init = var_decl
            .init
            .as_ref()
            .map(|init| -> Result<String> {
                Ok(format!(" = *({}).value", self.transpile_expr(&init)?))
            })
            .unwrap_or(Ok("".to_string()))?;
        Ok(format!("JSValue {}{}", ident.sym, init))
    }

    fn transpile_expr(&mut self, expr: &Expr) -> Result<String> {
        match expr {
            Expr::Ident(ident) => Ok(format!("{}", ident.sym)),
            Expr::Lit(literal) => self.transpile_literal(literal),
            Expr::Array(array_lit) => self.transpile_array_literal(array_lit),
            Expr::Call(call_expr) => self.transpile_call_expr(call_expr),
            Expr::Member(member_expr) => self.transpile_member_expr(member_expr),
            Expr::Arrow(arrow_expr) => self.transpile_arrow_expr(arrow_expr),
            Expr::Bin(bin_expr) => self.transpile_bin_expr(bin_expr),
            Expr::Tpl(tpl_expr) => self.transpile_tpl_expr(tpl_expr),
            Expr::TaggedTpl(tagged_tpl_expr) => self.transpile_tagged_tpl_expr(tagged_tpl_expr),
            Expr::Object(object_lit) => self.transpile_object_lit(object_lit),
            Expr::Paren(paren_expr) => self.transpile_paren_expr(paren_expr),
            Expr::Fn(fn_expr) => self.transpile_fn_expr(fn_expr),
            Expr::This(this_expr) => self.transpile_this_expr(this_expr),
            Expr::Assign(assign_expr) => self.transpile_assign_expr(assign_expr),
            Expr::Cond(cond_expr) => self.transpile_cond_expr(cond_expr),
            Expr::Update(update_expr) => self.transpile_update_expr(update_expr),
            Expr::Yield(yield_expr) => self.transpile_yield_expr(yield_expr),
            _ => Err(anyhow!("Unsupported expression {:?}", expr)),
        }
    }

    fn transpile_yield_expr(&mut self, yield_expr: &YieldExpr) -> Result<String> {
        if yield_expr.delegate {
            return Err(anyhow!("No support for delegate yields yet."));
        }
        let value = yield_expr
            .arg
            .as_ref()
            .map(|arg| self.transpile_expr(arg))
            .transpose()?
            .unwrap_or(format!("JSValue::undefined()"));
        Ok(format!("co_yield {}", value))
    }

    fn transpile_update_expr(&mut self, update_expr: &UpdateExpr) -> Result<String> {
        let expr = self.transpile_expr(update_expr.arg.as_ref())?;
        let op = match update_expr.op {
            UpdateOp::MinusMinus => "--",
            UpdateOp::PlusPlus => "++",
        };
        Ok(match update_expr.prefix {
            true => format!("{}({})", op, expr),
            false => format!("({}){}", expr, op),
        })
    }

    fn transpile_cond_expr(&mut self, cond_expr: &CondExpr) -> Result<String> {
        let test = self.transpile_expr(&cond_expr.test)?;
        let cons = self.transpile_expr(&cond_expr.cons)?;
        let alt = self.transpile_expr(&cond_expr.alt)?;
        Ok(format!("({}).coerce_to_bool()?({}):({})", test, cons, alt))
    }

    fn transpile_assign_expr(&mut self, assign_expr: &AssignExpr) -> Result<String> {
        self.is_lhs = true;
        let left = match &assign_expr.left {
            PatOrExpr::Expr(expr) => self.transpile_expr(expr)?,
            PatOrExpr::Pat(pat) => match pat.as_ref() {
                Pat::Expr(expr) => self.transpile_expr(expr)?,
                Pat::Ident(ident) => format!("{}", ident.sym),
                _ => {
                    return Err(anyhow!(
                        "Unsupported assignment pattern {:?}",
                        assign_expr.left
                    ))
                }
            },
        };
        self.is_lhs = false;
        let right = self.transpile_expr(&assign_expr.right)?;
        let op = match assign_expr.op {
            AssignOp::Assign => "=",
            _ => return Err(anyhow!("Unsupported assign operation {:?}", assign_expr.op)),
        };
        Ok(format!("{} {} *({}).value", left, op, right))
    }

    fn transpile_this_expr(&mut self, _this_expr: &ThisExpr) -> Result<String> {
        Ok(format!("thisArg"))
    }

    fn transpile_function(&mut self, function: &Function) -> Result<String> {
        if function.is_async {
            return Err(anyhow!("Async functions not supported yet"));
        }
        if function.is_generator {
            return self.transpile_generator_function(function);
        }
        return self.transpile_plain_function(function);
    }

    fn transpile_generator_function(&mut self, function: &Function) -> Result<String> {
        assert!(function.is_generator);
        self.is_generator = true;
        let param_destructure =
            self.transpile_param_destructure(function.params.iter().map(|param| &param.pat))?;
        let body = match &function.body {
            Some(block_stmt) => self.transpile_block_stmt(block_stmt)?,
            _ => return Err(anyhow!("Function lacks a body")),
        };
        self.is_generator = false;
        Ok(format!(
            "JSValue::new_generator_function([=](JSValue thisArg, std::vector<JSValue>& args) mutable -> JSGeneratorAdapter {{
                    {}
                    {}
                    co_return;
                }})",
            param_destructure, body
        ))
    }

    fn transpile_plain_function(&mut self, function: &Function) -> Result<String> {
        let param_destructure =
            self.transpile_param_destructure(function.params.iter().map(|param| &param.pat))?;
        let body = match &function.body {
            Some(block_stmt) => self.transpile_block_stmt(block_stmt)?,
            _ => return Err(anyhow!("Function lacks a body")),
        };
        Ok(format!(
            "JSValue::new_function([=](JSValue thisArg, std::vector<JSValue>& args) mutable -> JSValue {{
                    {}
                    {}
                    return JSValue::undefined();
                }})",
            param_destructure, body
        ))
    }

    fn transpile_fn_expr(&mut self, fn_expr: &FnExpr) -> Result<String> {
        self.transpile_function(&fn_expr.function)
    }

    fn transpile_paren_expr(&mut self, paren_expr: &ParenExpr) -> Result<String> {
        Ok(format!("({})", self.transpile_expr(&paren_expr.expr)?))
    }

    fn transpile_object_lit(&mut self, object_lit: &ObjectLit) -> Result<String> {
        let transpiled_props: Vec<Result<String>> = object_lit
            .props
            .iter()
            .map(|prop| match prop {
                PropOrSpread::Spread(_) => return Err(anyhow!("Object spread unsupported")),
                PropOrSpread::Prop(prop) => match prop.as_ref() {
                    Prop::Shorthand(ident) => self.transpile_prop_shorthand(ident),
                    Prop::KeyValue(key_value) => self.transpile_prop_keyvalue(key_value),
                    Prop::Getter(getter) => self.transpile_prop_getter(getter),
                    Prop::Setter(setter) => self.transpile_prop_setter(setter),
                    Prop::Method(method) => self.transpile_prop_method(method),
                    _ => Err(anyhow!("Unsupported object property {:?}", prop)),
                },
            })
            .collect();
        let prop_defs = Result::<Vec<String>>::from_iter(transpiled_props)?.join(",\n");
        Ok(format!("JSValue::new_object({{ {} }})", prop_defs))
    }

    fn transpile_prop_method(&mut self, method: &MethodProp) -> Result<String> {
        Ok(format!(
            "{{ {}, {} }}",
            self.transpile_prop_name(&method.key)?,
            self.transpile_function(&method.function)?
        ))
    }

    fn transpile_prop_setter(&mut self, setter: &SetterProp) -> Result<String> {
        let ident = setter
            .param
            .as_ident()
            .ok_or(anyhow!("Setter parameter must be an ident"))?;
        Ok(format!(
            r#"{{
                {},
                JSValueBinding::with_getter_setter(
                    JSValue::undefined(),
                    JSValue::new_function([=](JSValue thisArg, std::vector<JSValue>& args) mutable -> JSValue {{
                        JSValue {} = args[0];
                        {}
                        return JSValue::undefined();
                    }})
                )
            }}"#,
            self.transpile_prop_name(&setter.key)?,
            ident.sym,
            self.transpile_block_stmt(setter.body.as_ref().ok_or(anyhow!("Getter needs a body"))?)?
        ))
    }

    fn transpile_prop_getter(&mut self, getter: &GetterProp) -> Result<String> {
        Ok(format!(
            r#"{{
                {},
                JSValueBinding::with_getter_setter(
                    JSValue::new_function([=](JSValue thisArg, std::vector<JSValue>& args) mutable -> JSValue {{
                        {}
                        return JSValue::undefined();
                    }}),
                    JSValue::undefined()
                )
            }}"#,
            self.transpile_prop_name(&getter.key)?,
            self.transpile_block_stmt(getter.body.as_ref().ok_or(anyhow!("Getter needs a body"))?)?
        ))
    }

    fn transpile_prop_keyvalue(&mut self, key_value: &KeyValueProp) -> Result<String> {
        Ok(format!(
            "{{{}, {}}}",
            self.transpile_prop_name(&key_value.key)?,
            self.transpile_expr(&key_value.value)?
        ))
    }

    fn transpile_prop_shorthand(&mut self, ident: &Ident) -> Result<String> {
        Ok(format!(r#"{{JSValue{{"{0}"}}, {0}}}"#, ident.sym))
    }

    fn transpile_prop_name(&mut self, prop_name: &PropName) -> Result<String> {
        match prop_name {
            PropName::Ident(ident) => Ok(format!(r#"JSValue{{"{}"}}"#, ident.sym)),
            PropName::Str(str) => {
                let v = str
                    .raw
                    .as_ref()
                    .map(|v| format!("{}", v))
                    .unwrap_or(format!("{}", str.value));
                Ok(format!(r#""{}""#, v))
            }
            PropName::Computed(computed_prop_name) => self.transpile_expr(&computed_prop_name.expr),
            _ => Err(anyhow!("Unsupported property name {:?}", prop_name)),
        }
    }

    fn transpile_tpl_expr(&mut self, tpl_expr: &Tpl) -> Result<String> {
        if tpl_expr.quasis.len() > 1 {
            return Err(anyhow!("No support for template string interpolation yet"));
        }
        Ok(format!(r#"JSValue{{"{}"}}"#, tpl_expr.quasis[0].raw))
    }

    fn transpile_tagged_tpl_expr(&mut self, tagged_tpl_expr: &TaggedTpl) -> Result<String> {
        let tag = self.transpile_expr(&tagged_tpl_expr.tag)?;
        let tpl = self.transpile_tpl_expr(&tagged_tpl_expr.tpl)?;
        if tag == "raw_cpp" {
            return Ok(tpl[1..tpl.len() - 1].to_string());
        }
        Err(anyhow!("No support for tagged template expressions"))
    }

    fn transpile_bin_expr(&mut self, bin_expr: &BinExpr) -> Result<String> {
        let left = self.transpile_expr(&bin_expr.left)?;
        let right = self.transpile_expr(&bin_expr.right)?;
        let op = match bin_expr.op {
            BinaryOp::Add => "+",
            BinaryOp::Mul => "*",
            BinaryOp::Gt => ">",
            BinaryOp::GtEq => ">=",
            BinaryOp::EqEq => "==",
            BinaryOp::EqEqEq => "==",
            BinaryOp::Lt => "<",
            BinaryOp::LtEq => "<=",
            BinaryOp::NotEq => "!=",
            BinaryOp::NotEqEq => "!=",
            BinaryOp::LogicalAnd => "&&",
            BinaryOp::LogicalOr => "||",
            BinaryOp::Mod => "%",
            _ => return Err(anyhow!("Unsupported binary operation {:?}", bin_expr.op)),
        };
        Ok(format!("({}){}({})", left, op, right))
    }

    fn transpile_param_destructure<'a>(
        &mut self,
        params: impl Iterator<Item = &'a Pat>,
    ) -> Result<String> {
        let transpiled_params: Vec<Result<String>> = params
            .enumerate()
            .map(|(idx, param)| {
                param
                    .as_ident()
                    .map(|ident| format!("JSValue {} = args[{}];", ident.sym, idx))
                    .ok_or(anyhow!(
                        "Only straight-up identifiers are supported as function parameters"
                    ))
            })
            .collect();
        Ok(Result::<Vec<String>>::from_iter(transpiled_params)?.join(";"))
    }

    fn transpile_block_stmt(&mut self, block_stmt: &BlockStmt) -> Result<String> {
        let stmts: Vec<Result<String>> = block_stmt
            .stmts
            .iter()
            .map(|stmt| self.transpile_stmt(stmt))
            .collect();
        let block: String = Result::<Vec<String>>::from_iter(stmts)?.join(";\n");
        Ok(format!("{{ {} }}", block))
    }

    fn transpile_arrow_expr(&mut self, arrow_expr: &ArrowExpr) -> Result<String> {
        let param_destructure = self.transpile_param_destructure(arrow_expr.params.iter())?;
        let body = match &arrow_expr.body {
            BlockStmtOrExpr::Expr(expr) => format!("return {};", self.transpile_expr(expr)?),
            BlockStmtOrExpr::BlockStmt(block_stmt) => self.transpile_block_stmt(block_stmt)?,
        };
        Ok(format!(
            "JSValue::new_function([=](JSValue thisArg, std::vector<JSValue>& args) mutable {{
                {}
                {}
            }})",
            param_destructure, body
        ))
    }

    fn transpile_member_expr(&mut self, member_expr: &MemberExpr) -> Result<String> {
        let obj = self.transpile_expr(&member_expr.obj)?;
        let prop = match &member_expr.prop {
            MemberProp::Ident(ident) => format!(r#"JSValue{{"{}"}}"#, ident.sym),
            MemberProp::Computed(computed_prop_name) => {
                self.transpile_expr(&computed_prop_name.expr)?
            }
            _ => return Err(anyhow!("Unsupported member prop {:?}", member_expr.prop)),
        };
        // Ok(if self.is_lhs {
        //     format!(r#"{}.get_property({})"#, obj, prop)
        // } else {
        Ok(format!(r#"{}[{}]"#, obj, prop))
        // })
    }

    fn transpile_call_expr(&mut self, call_expr: &CallExpr) -> Result<String> {
        let callee = self.transpile_expr(
            call_expr
                .callee
                .as_expr()
                .ok_or(anyhow!("Unsupported callee expr {:?}", call_expr.callee))?,
        )?;
        let transpiled_args: Vec<Result<String>> = call_expr
            .args
            .iter()
            .map(|arg| Ok(format!("*({}).value", self.transpile_expr(&arg.expr)?)))
            .collect();
        let arg_expr = Result::<Vec<String>>::from_iter(transpiled_args)?.join(",");
        Ok(format!("{}({{{}}})", callee, arg_expr))
    }

    fn transpile_array_literal(&mut self, array_lit: &ArrayLit) -> Result<String> {
        let transpiled_elems: Vec<Result<String>> = array_lit
            .elems
            .iter()
            .map(|item| {
                let item = item
                    .as_ref()
                    .ok_or(anyhow!("No support for empty array items"))?;
                self.transpile_expr(&item.expr)
            })
            .collect();

        Ok(format!(
            "JSValue::new_array({{{}}})",
            Result::<Vec<String>>::from_iter(transpiled_elems)?.join(",")
        ))
    }

    fn transpile_literal(&mut self, lit: &Lit) -> Result<String> {
        match lit {
            Lit::Num(num) => self.transpile_number(num),
            Lit::Str(str) => self.transpile_string(str),
            Lit::Bool(bool) => self.transpile_bool(bool),
            _ => Err(anyhow!("Unsupported literal {:?}", lit)),
        }
    }

    fn transpile_bool(&mut self, bool: &Bool) -> Result<String> {
        let bool_str = match bool.value {
            true => "true",
            false => "false",
        };

        Ok(format!("JSValue{{{}}}", bool_str))
    }
    fn transpile_string(&mut self, string: &Str) -> Result<String> {
        Ok(format!(
            r#"JSValue{{"{}"}}"#,
            string
                .raw
                .as_ref()
                .map(|v| format!("{}", &v[1..v.len() - 1]))
                .unwrap_or(format!("{}", string.value))
        ))
    }

    fn transpile_number(&mut self, num: &Number) -> Result<String> {
        Ok(format!("JSValue{{static_cast<double>({})}}", num.value))
    }
}
