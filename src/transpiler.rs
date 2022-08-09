use anyhow::{anyhow, Result};
use swc_ecma_ast::{
    ArrayLit, ArrowExpr, BinExpr, BinaryOp, BlockStmtOrExpr, CallExpr, Decl, Expr, ExprOrSpread,
    Lit, MemberExpr, MemberProp, Module, ModuleItem, Number, Pat, Stmt, VarDecl, VarDeclKind,
    VarDeclarator,
};

pub struct Transpiler {}

impl Transpiler {
    pub fn transpile_module(&mut self, module: &Module) -> Result<String> {
        let transpiled_items: Vec<Result<String>> = module
            .body
            .iter()
            .map(|item| -> Result<String> {
                match item {
                    ModuleItem::ModuleDecl(decl) => {
                        return Err(anyhow!("Module imports/exports are not support"))
                    }
                    ModuleItem::Stmt(stmt) => self.transpile_stmt(stmt),
                }
            })
            .collect();
        Ok(format!(
            r#"
		#include "runtime/js_value.h";

		int main() {{
			{}
			return 0;
		}}
		"#,
            Result::<Vec<String>>::from_iter(transpiled_items)?.join("\n")
        ))
    }

    fn transpile_stmt(&mut self, stmt: &Stmt) -> Result<String> {
        match stmt {
            Stmt::Decl(decl) => self.transpile_decl(decl),
            Stmt::Expr(expr_stmt) => self.transpile_expr(&expr_stmt.expr),
            _ => return Err(anyhow!("Unsupported statemt: {:?}", stmt)),
        }
    }

    fn transpile_decl(&mut self, decl: &Decl) -> Result<String> {
        match decl {
            Decl::Var(var_decl) => self.transpile_var_decl(var_decl),
            _ => return Err(anyhow!("Unsupported declaration: {:?}", decl)),
        }
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
            .map(|init| self.transpile_expr(&init))
            .unwrap_or(Ok("".to_string()))?;
        Ok(format!("auto {} = {};", ident.sym, init))
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
            _ => Err(anyhow!("Unsupported expression {:?}", expr)),
        }
    }

    fn transpile_bin_expr(&mut self, bin_expr: &BinExpr) -> Result<String> {
        let left = self.transpile_expr(&bin_expr.left)?;
        let right = self.transpile_expr(&bin_expr.right)?;
        let op = match bin_expr.op {
            BinaryOp::Add => "+",
            BinaryOp::Mul => "*",
            _ => return Err(anyhow!("Unsupported binary operation {:?}", bin_expr.op)),
        };
        Ok(format!("{}{}{}", left, op, right))
    }

    fn transpile_arrow_expr(&mut self, arrow_expr: &ArrowExpr) -> Result<String> {
        let transpiled_params: Vec<Result<String>> = arrow_expr
            .params
            .iter()
            .map(|param| {
                param
                    .as_ident()
                    .map(|ident| format!("{}", ident.sym))
                    .ok_or(anyhow!(
                        "Only straight-up identifiers are supported as function parameters"
                    ))
            })
            .collect();
        let param_list = Result::<Vec<String>>::from_iter(transpiled_params)?.join(", ");
        let body = match &arrow_expr.body {
            BlockStmtOrExpr::Expr(expr) => format!("{{ return {}; }}", self.transpile_expr(expr)?),
            _ => return Err(anyhow!("Unsupported body {:?}", arrow_expr.body)),
        };
        Ok(format!("[=]({}){}", param_list, body))
    }

    fn transpile_member_expr(&mut self, member_expr: &MemberExpr) -> Result<String> {
        let obj = self.transpile_expr(&member_expr.obj)?;
        let prop = match &member_expr.prop {
            MemberProp::Ident(ident) => format!("{}", ident.sym),
            _ => return Err(anyhow!("Unsupported member prop {:?}", member_expr.prop)),
        };
        Ok(format!(r#"{}["{}"]"#, obj, prop))
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
        Ok(format!("{}({})", callee, arg_expr))
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

    // fn transpile_expr_or_spread(&mut self, expr_or_spread: &ExprOrSpread) -> Result<String> {
    // 	match expr_or_spread {
    // 		ExprOrSpread
    // 	}
    // }

    fn transpile_literal(&mut self, lit: &Lit) -> Result<String> {
        match lit {
            Lit::Num(num) => self.transpile_number(num),
            _ => Err(anyhow!("Unsupported literal {:?}", lit)),
        }
    }

    fn transpile_number(&mut self, num: &Number) -> Result<String> {
        Ok(format!("JSValue{{static_cast<double>({})}}", num.value))
    }
}
