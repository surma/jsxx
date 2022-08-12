use std::collections::HashSet;

use anyhow::{anyhow, Result};
use swc_ecma_ast::*;

pub struct Transpiler {
    pub globals: Vec<crate::globals::Global>,
    is_lhs: bool,
}

impl Transpiler {
    pub fn new() -> Transpiler {
        Transpiler {
            globals: vec![],
            is_lhs: false,
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
            program = Result::<Vec<String>>::from_iter(transpiled_items)?.join("\n")
        ))
    }

    fn transpile_stmt(&mut self, stmt: &Stmt) -> Result<String> {
        match stmt {
            Stmt::Decl(decl) => self.transpile_decl(decl),
            Stmt::Expr(expr_stmt) => Ok(format!("{};", self.transpile_expr(&expr_stmt.expr)?)),
            Stmt::Block(block_stmt) => self.transpile_block_stmt(block_stmt),
            Stmt::Return(return_stmt) => self.transpile_return_stmt(return_stmt),
            Stmt::If(if_stmt) => self.transpile_if_stmt(if_stmt),
            _ => return Err(anyhow!("Unsupported statemt: {:?}", stmt)),
        }
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
        Ok(format!("return {};", arg))
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
        let transpiled_var_decl: Vec<Result<String>> = var_decl
            .decls
            .iter()
            .map(|decl| self.transpile_var_declarator(decl))
            .collect();
        Ok(Result::<Vec<String>>::from_iter(transpiled_var_decl)?.join("\n"))
    }

    fn transpile_var_declarator(&mut self, var_decl: &VarDeclarator) -> Result<String> {
        let ident = var_decl.name.as_ident().ok_or(anyhow!(
            "Only straight-up identifiers are supported for variable declarations for now."
        ))?;
        let init = var_decl
            .init
            .as_ref()
            .map(|init| -> Result<String> { Ok(format!(" = {}", self.transpile_expr(&init)?)) })
            .unwrap_or(Ok("".to_string()))?;
        Ok(format!("JSValue {}{};", ident.sym, init))
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
            _ => Err(anyhow!("Unsupported expression {:?}", expr)),
        }
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
        Ok(format!("{} {} {}", left, op, right))
    }

    fn transpile_this_expr(&mut self, _this_expr: &ThisExpr) -> Result<String> {
        Ok(format!("thisArg"))
    }

    fn transpile_function(&mut self, function: &Function) -> Result<String> {
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
                    _ => Err(anyhow!("Unsupported object property {:?}", prop)),
                },
            })
            .collect();
        let prop_defs = Result::<Vec<String>>::from_iter(transpiled_props)?.join(",\n");
        Ok(format!("JSValue::new_object({{ {} }})", prop_defs))
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
            "{{{}, JSValueBinding::with_value({})}}",
            self.transpile_prop_name(&key_value.key)?,
            self.transpile_expr(&key_value.value)?
        ))
    }

    fn transpile_prop_shorthand(&mut self, ident: &Ident) -> Result<String> {
        Ok(format!(
            r#"{{JSValue{{"{0}"}}, JSValueBinding::with_value({0})}}"#,
            ident.sym
        ))
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
            _ => return Err(anyhow!("Unsupported body {:?}", arrow_expr.body)),
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
        Ok(if self.is_lhs {
            format!(r#"{}.get_property_slot({})"#, obj, prop)
        } else {
            format!(r#"{}[{}]"#, obj, prop)
        })
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
            .map(|arg| self.transpile_expr(&arg.expr))
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
            _ => Err(anyhow!("Unsupported literal {:?}", lit)),
        }
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
