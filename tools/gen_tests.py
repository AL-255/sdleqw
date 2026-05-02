#!/usr/bin/env python3
"""
gen_tests.py — emit twin test scripts for x50ng and sdleqw.

Mini-language (sdleqw side, run by `./sdleqw --batch`):

  Letters and digits, plus '.': literal char insertion.
  Binary ops: + - * /
  '^' power, '@' sqrt, '#' nth-root,
  '(' parens wrap, '|' abs wrap,
  ',' (or space): next argument.
  Escapes (always use $-prefix for operations that aren't symbols):
    $l left, $r right, $u up, $k down
    $e ENTER, $x ESC, $b BACKSPACE, $t TAB
    $D DEL, $K CLEAR-arg-of-2-arg, $L CLEAR-all-but-selection
    $z zoom toggle
    $s SIN $c COS $T TAN $n LN $X EXP $g LOG $a ABS
    $I integral, $S sum, $P partial-deriv, $W where, $Z complex, $F userfunc

The x50ng side compiles each logical action to TAPs of HP4950_KEY_*. Letters
emit a one-shot ALPHA before each letter (HP's alpha-lock semantics did not
register reliably in our timing).
"""

import os, sys

SCRIPTS_DIR = os.path.join(os.path.dirname(__file__), "..", "scripts")
os.makedirs(SCRIPTS_DIR, exist_ok=True)

LETTER_KEY = {
    "A": "A", "B": "B", "C": "C", "D": "D", "E": "E", "F": "F",
    "G": "G", "H": "H", "I": "I", "J": "J", "K": "K", "L": "L",
    "M": "M", "N": "N", "O": "O", "P": "P", "Q": "Q", "R": "R",
    "S": "S", "T": "T", "U": "U", "V": "V", "W": "W", "Y": "Y",
    "Z": "Z",
}

class X50Builder:
    def __init__(self):
        self.lines = []

    def emit_digit(self, c):       self.lines.append(f"TAP {c}")
    def emit_period(self):         self.lines.append("TAP PERIOD")
    def emit_letter(self, c):
        c = c.upper()
        if c == "X":
            self.lines.append("TAP X")
            return
        if c not in LETTER_KEY:
            raise ValueError(f"no letter mapping for {c!r}")
        self.lines.append("TAP ALPHA")
        self.lines.append(f"TAP {LETTER_KEY[c]}")
    def emit_op_plus(self):        self.lines.append("TAP PLUS")
    def emit_op_minus(self):       self.lines.append("TAP MINUS")
    def emit_op_mul(self):         self.lines.append("TAP MULTIPLY")
    def emit_op_div(self):         self.lines.append("TAP Z")
    def emit_op_pow(self):         self.lines.append("TAP Q")
    def emit_sqrt(self):           self.lines.append("TAP R")
    def emit_nthroot(self):
        self.lines.append("TAP RIGHTSHIFT"); self.lines.append("TAP R")
    def emit_paren(self):
        self.lines.append("TAP LEFTSHIFT"); self.lines.append("TAP Z")
    def emit_abs(self):
        # Type "ABS(" via alpha
        self.emit_letter("A"); self.emit_letter("B"); self.emit_letter("S")
        self.emit_paren()
    def emit_integral(self):
        self.lines.append("TAP RIGHTSHIFT"); self.lines.append("TAP U")
    def emit_sum(self):
        self.lines.append("TAP RIGHTSHIFT"); self.lines.append("TAP S")
    def emit_deriv(self):
        self.lines.append("TAP RIGHTSHIFT"); self.lines.append("TAP T")
    def emit_where(self):
        # α LS VAR (per doc 8.2.17)
        self.lines.append("TAP ALPHA")
        self.lines.append("TAP LEFTSHIFT"); self.lines.append("TAP J")
    def emit_complex(self):
        self.lines.append("TAP LEFTSHIFT"); self.lines.append("TAP PERIOD")
    def emit_userfunc(self):
        self.lines.append("TAP LEFTSHIFT"); self.lines.append("TAP Z")
    def emit_next_arg(self):       self.lines.append("TAP SPACE")
    def emit_left(self):           self.lines.append("TAP LEFT")
    def emit_right(self):          self.lines.append("TAP RIGHT")
    def emit_up(self):             self.lines.append("TAP UP")
    def emit_down(self):           self.lines.append("TAP DOWN")
    def emit_backspace(self):      self.lines.append("TAP BACKSPACE")
    def emit_del(self):
        self.lines.append("TAP LEFTSHIFT"); self.lines.append("TAP BACKSPACE")
    def emit_clear(self):
        self.lines.append("TAP RIGHTSHIFT"); self.lines.append("TAP BACKSPACE")
    def emit_func_apply(self, name):
        if name == "SIN": self.lines.append("TAP S")
        elif name == "COS": self.lines.append("TAP T")
        elif name == "TAN": self.lines.append("TAP U")
        elif name == "LN":  self.lines.append("TAP RIGHTSHIFT"); self.lines.append("TAP Q")
        elif name == "EXP": self.lines.append("TAP LEFTSHIFT");  self.lines.append("TAP Q")
        elif name == "LOG": self.lines.append("TAP RIGHTSHIFT"); self.lines.append("TAP V")
        elif name == "ABS": self.emit_abs()
        else: raise ValueError(name)
    def emit_zoom(self):
        self.lines.append("TAP LEFTSHIFT"); self.lines.append("TAP N")
    def emit_enter(self): self.lines.append("TAP ENTER")
    def emit_esc(self):   self.lines.append("TAP ON")

    def lower(self, s):
        i = 0
        while i < len(s):
            c = s[i]
            if c == "$":
                i += 1
                esc = s[i]
                {
                    "l": self.emit_left, "r": self.emit_right,
                    "u": self.emit_up,   "k": self.emit_down,
                    "e": self.emit_enter, "x": self.emit_esc,
                    "b": self.emit_backspace, "t": (lambda: None),
                    "D": self.emit_del, "K": self.emit_clear,
                    "L": self.emit_clear, "z": self.emit_zoom,
                    "s": (lambda: self.emit_func_apply("SIN")),
                    "c": (lambda: self.emit_func_apply("COS")),
                    "T": (lambda: self.emit_func_apply("TAN")),
                    "n": (lambda: self.emit_func_apply("LN")),
                    "X": (lambda: self.emit_func_apply("EXP")),
                    "g": (lambda: self.emit_func_apply("LOG")),
                    "a": (lambda: self.emit_func_apply("ABS")),
                    "I": self.emit_integral, "S": self.emit_sum,
                    "P": self.emit_deriv, "W": self.emit_where,
                    "Z": self.emit_complex, "F": self.emit_userfunc,
                }[esc]()
                i += 1
                continue
            if c.isdigit():       self.emit_digit(c)
            elif c == ".":        self.emit_period()
            elif c == "+":        self.emit_op_plus()
            elif c == "-":        self.emit_op_minus()
            elif c == "*":        self.emit_op_mul()
            elif c == "/":        self.emit_op_div()
            elif c == "^":        self.emit_op_pow()
            elif c == "@":        self.emit_sqrt()
            elif c == "#":        self.emit_nthroot()
            elif c == "(":        self.emit_paren()
            elif c == "|":        self.emit_abs()
            elif c == ",":        self.emit_next_arg()
            elif c == " ":        self.emit_next_arg()
            elif c.isalpha():     self.emit_letter(c)
            else: raise ValueError(f"unknown key char {c!r}")
            i += 1
        return self.lines

# ----- test list -----
TESTS = []
def T(name, keys):
    TESTS.append((name, keys))

# --- 1) atomic numbers (~30) ---
for d in "0123456789":
    T(f"num_{d}", d)
T("num_10", "10")
T("num_42", "42")
T("num_100", "100")
T("num_1234", "1234")
T("num_12345", "12345")
T("num_3p14", "3.14")
T("num_0p5", "0.5")
T("num_2p0", "2.0")
T("num_7p5", "7.5")
T("num_0p001", "0.001")
T("num_99p99", "99.99")
T("num_1000000", "1000000")
T("num_5p0", "5.0")
T("num_1p0", "1.0")
T("num_25", "25")
T("num_360", "360")
T("num_180", "180")
T("num_2p5", "2.5")
T("num_neg_5", "-5")
T("num_neg_3p14", "-3.14")
T("num_dot_5", ".5")
T("num_99", "99")
T("num_1000", "1000")
T("num_500", "500")

# --- 2) atomic names (~30) ---
for c in "ABEFGHJKLMNORTYZ":
    T(f"name_{c}", c)
for c in "abefghjklmnortyz":
    T(f"name_{c}", c)
T("name_X", "X")
T("name_x", "x")
T("name_xy", "xy")
T("name_abc", "ABC")
T("name_t", "t")
T("name_R", "R")
T("name_b", "b")
T("name_PI", "PI")
T("name_OMEGA", "OMEGA")
T("name_THETA", "THETA")
T("name_var1", "var1")
T("name_X1", "X1")
T("name_a1", "a1")
T("name_FOO", "FOO")
T("name_BAR", "BAR")
T("name_PHI", "PHI")
T("name_zeta", "zeta")
T("name_eta", "eta")
T("name_NUMBER", "NUMBER")
T("name_value", "value")
T("name_alpha", "alpha")
T("name_beta", "beta")
T("name_gamma", "gamma")
T("name_delta", "delta")
T("name_long", "VARIABLE")
T("name_kappa", "kappa")

# --- 3) addition (~50) ---
T("add_1_1", "1+1")
T("add_1_2", "1+2")
T("add_5_3", "5+3")
T("add_X_Y", "X+Y")
T("add_X_5", "X+5")
T("add_5_X", "5+X")
T("add_a_b", "A+B")
T("add_3_3_3", "3+3+3")
T("add_5_X_2", "5+X+2")
T("add_1p5_2p5", "1.5+2.5")
T("add_3p14_2p71", "3.14+2.71")
T("add_X_Y_Z", "X+Y+Z")
T("add_long", "1+2+3+4+5")
T("add_two_names", "FOO+BAR")
T("add_zero", "0+5")
T("add_decimal", "0.1+0.2")
T("add_X_X", "X+X")
T("add_t_dt", "t+dt")
T("add_neg_5_3", "-5+3")
T("add_3_neg_5", "3+-5")
T("add_long_decimal", "1.234+5.678")
T("add_var_with_index", "X1+X2")
T("add_var_const", "X+1")
T("add_var_neg_const", "X+-1")
T("add_PI_2", "PI+2")
T("add_X_PI", "X+PI")
T("add_E_M", "E+M")
T("add_p_q", "p+q")
T("add_n_1", "n+1")
T("add_2_n", "2+n")
T("add_six_terms", "1+2+3+4+5+6")
T("add_N_M_K", "N+M+K")
T("add_const_var", "5+L")
T("add_var_var_var", "A+B+R")
T("add_three_const", "10+20+30")
T("add_three_decimals", "1.1+2.2+3.3")
T("add_neg_neg", "-1+-2")
T("add_X_neg_Y", "X+-Y")
T("add_Y_X", "Y+X")
T("add_x_x_x", "x+x+x")
T("add_M_n", "M+n")
T("add_u_v", "u+v")
T("add_ten_const", "10+10")
T("add_decim_5_5", "5.5+5.5")
T("add_long_var_const", "VARIABLE+1")
T("add_bigname_const", "ALPHA+1")
T("add_bigname_bigname", "ALPHA+BETA")
T("add_X_neg_3", "X+-3")
T("add_neg_X_5", "-X+5")
T("add_R_y", "R+y")

# --- 4) subtraction (~30) ---
T("sub_5_3", "5-3")
T("sub_X_Y", "X-Y")
T("sub_X_5", "X-5")
T("sub_X_1", "X-1")
T("sub_a_b", "A-B")
T("sub_long", "10-3-2")
T("sub_decimal", "1.5-0.5")
T("sub_negative", "-1-1")
T("sub_X_neg_Y", "X--Y")
T("sub_5_X_3", "5-X-3")
T("sub_n_1", "n-1")
T("sub_p_q", "p-q")
T("sub_R_y", "R-y")
T("sub_2_X", "2-X")
T("sub_X_2", "X-2")
T("sub_PI_3", "PI-3")
T("sub_X_X", "X-X")
T("sub_a_a_a", "A-A-A")
T("sub_x_PI", "x-PI")
T("sub_X_one", "X-1")
T("sub_X_zero", "X-0")
T("sub_six_minus_two", "6-2")
T("sub_four_minus_one", "4-1")
T("sub_seven_minus_three", "7-3")
T("sub_eight_minus_four", "8-4")
T("sub_nine_minus_five", "9-5")
T("sub_Z_R", "Z-R")
T("sub_long_var", "VARIABLE-1")
T("sub_neg_const", "-3-X")

# --- 5) multiplication (~30) ---
T("mul_2_3", "2*3")
T("mul_5_X", "5*X")
T("mul_X_5", "X*5")
T("mul_X_Y", "X*Y")
T("mul_a_b_c", "A*B*R")
T("mul_2_PI", "2*PI")
T("mul_3_4", "3*4")
T("mul_decimals", "1.5*2.5")
T("mul_neg", "-2*3")
T("mul_X_2", "X*2")
T("mul_2_X_3", "2*X*3")
T("mul_n_n", "n*n")
T("mul_X_X_X", "X*X*X")
T("mul_3_PI_R", "3*PI*R")
T("mul_R_t", "R*t")
T("mul_with_long", "RHO*V")
T("mul_const_const", "10*10")
T("mul_lots", "1*2*3*4")
T("mul_PI_R_R", "PI*R*R")
T("mul_decimal_var", "0.5*X")
T("mul_2_n", "2*n")
T("mul_X_neg_1", "X*-1")
T("mul_neg_X", "-X*2")
T("mul_alpha_beta", "ALPHA*BETA")
T("mul_xyz", "x*y*z")
T("mul_pqr", "p*q*r")
T("mul_2_L", "2*L")
T("mul_3_OMEGA", "3*OMEGA")
T("mul_THETA_2", "THETA*2")
T("mul_zero_X", "0*X")

# --- 6) division (~30) ---
T("div_1_2", "1/2")
T("div_2_3", "2/3")
T("div_X_Y", "X/Y")
T("div_5_X", "5/X")
T("div_X_5", "X/5")
T("div_X_X", "X/X")
T("div_chain", "1/2/3")
T("div_decimals", "3.14/2")
T("div_a_b", "A/B")
T("div_PI_2", "PI/2")
T("div_2_PI", "2/PI")
T("div_X_2", "X/2")
T("div_X_neg_2", "X/-2")
T("div_neg_X_5", "-X/5")
T("div_one_X", "1/X")
T("div_X_one", "X/1")
T("div_t_5", "t/5")
T("div_5_t", "5/t")
T("div_R_y", "R/y")
T("div_long_var", "VARIABLE/2")
T("div_const_var", "10/X")
T("div_var_const", "X/10")
T("div_two_var", "X/Y")
T("div_p_q", "p/q")
T("div_n_2", "n/2")
T("div_2_n", "2/n")
T("div_F_t", "F/t")
T("div_a_b_c", "A/B/R")
T("div_xyz", "x/y/z")
T("div_neg_a_b", "-A/B")

# --- 7) powers (~30) ---
T("pow_X_2", "X^2")
T("pow_X_3", "X^3")
T("pow_2_X", "2^X")
T("pow_X_n", "X^n")
T("pow_X_Y", "X^Y")
T("pow_e_X", "e^X")
T("pow_X_2_plus_1", "X^2+1")
T("pow_pow", "X^Y^Z")
T("pow_decimal", "X^0.5")
T("pow_neg", "X^-1")
T("pow_N_2", "N^2")
T("pow_R_2", "R^2")
T("pow_t_2", "t^2")
T("pow_y_3", "y^3")
T("pow_a_b", "A^B")
T("pow_PI_X_2", "PI*X^2")
T("pow_2_2_2", "2^2^2")
T("pow_X_4", "X^4")
T("pow_X_5", "X^5")
T("pow_X_6", "X^6")
T("pow_X_10", "X^10")
T("pow_X_neg_2", "X^-2")
T("pow_X_X", "X^X")
T("pow_n_k", "n^k")
T("pow_x_third", "X^0.33")
T("pow_2_n_minus_1", "2^n-1")
T("pow_n_n", "n^n")
T("pow_OMEGA_X", "OMEGA*X")
T("pow_a_inv", "a^-1")
T("pow_var_2", "VARIABLE^2")

# --- 8) sqrt (~20) ---
T("sqrt_just", "@")
T("sqrt_X_after", "X@")     # auto-mul: X * sqrt(?)
T("sqrt_then_X", "@X")
T("sqrt_2_after", "2@")
T("sqrt_3_after", "3@")
T("sqrt_then_2", "@2")
T("sqrt_then_4", "@4")
T("sqrt_X_squared", "@X^2")
T("sqrt_then_sum", "@1+1")
T("sqrt_X_plus_Y", "@X+Y")
T("sqrt_a_div_b", "@A/B")
T("sqrt_X_plus_1", "@X+1")
T("sqrt_PI", "@PI")
T("sqrt_long", "@VARIABLE")
T("sqrt_in_div", "@4/2")
T("sqrt_R", "@R")
T("sqrt_t", "@t")
T("sqrt_after_n", "@n")
T("sqrt_after_long_name", "@ALPHA")
T("sqrt_after_minus", "-@4")

# --- 9) nth-root (~10) ---
T("nthroot_3_X", "#3$rX")
T("nthroot_2_4", "#2$r4")
T("nthroot_3_8", "#3$r8")
T("nthroot_4_16", "#4$r16")
T("nthroot_5_X", "#5$rX")
T("nthroot_n_X", "#n$rX")
T("nthroot_3_a", "#3$ra")
T("nthroot_X_2", "#X$r2")
T("nthroot_2_X_squared", "#2$rX^2")
T("nthroot_n_2", "#n$r2")

# --- 10) explicit parens (~15) ---
T("paren_around_X", "X(")
T("paren_around_5", "5(")
T("paren_around_a_plus_b", "A+B(")
T("paren_around_X_div_Y", "X/Y(")
T("paren_double", "X((")
T("paren_then_op", "X(+1")
T("paren_around_sum", "X+Y(")
T("paren_around_diff", "X-Y(")
T("paren_around_pow", "X^2(")
T("paren_around_decimal", "3.14(")
T("paren_around_neg_X", "-X(")
T("paren_X_div_2", "X(/2")
T("paren_around_abc", "ABC(")
T("paren_around_PI", "PI(")
T("paren_around_long", "VARIABLE(")

# --- 11) abs (~10) ---
T("abs_X", "X|")
T("abs_5", "5|")
T("abs_X_plus_Y", "X+Y|")
T("abs_X_minus_5", "X-5|")
T("abs_X_squared", "X^2|")
T("abs_X_div_Y", "X/Y|")
T("abs_PI", "PI|")
T("abs_neg_X", "-X|")
T("abs_long_name", "VARIABLE|")
T("abs_a_plus_b", "A+B|")

# --- 12) prefix functions (~35) ---
T("sin_X", "X$s")
T("sin_2", "2$s")
T("sin_PI", "PI$s")
T("cos_X", "X$c")
T("cos_2", "2$c")
T("tan_X", "X$T")
T("ln_X", "X$n")
T("ln_e", "e$n")
T("exp_X", "X$X")
T("exp_neg_X", "-X$X")
T("log_X", "X$g")
T("log_10", "10$g")
T("absfn_X", "X$a")
T("sin_X_plus_Y", "X+Y$s")
T("cos_X_plus_Y", "X+Y$c")
T("sin_X_div_2", "X/2$s")
T("cos_X_2", "X^2$c")
T("ln_X_plus_1", "X+1$n")
T("sin_5", "5$s")
T("sin_neg_X", "-X$s")
T("ln_2", "2$n")
T("exp_2X", "2*X$X")
T("sin_X_squared", "X^2$s")
T("cos_X_squared", "X^2$c")
T("tan_X_plus_PI", "X+PI$T")
T("log_PI", "PI$g")
T("ln_PI", "PI$n")
T("exp_pi_x", "PI*X$X")
T("sin_PI_2", "PI/2$s")
T("cos_PI_2", "PI/2$c")
T("sin_X_minus_1", "X-1$s")
T("ln_VARIABLE", "VARIABLE$n")
T("sin_then_paren_X", "X$s(")
T("sin_X_times_2", "X*2$s")
T("ln_X_X", "X*X$n")

# --- 13) integrals (~25) ---
T("integ_basic", "$I0$rX$rX$rX")
T("integ_0_to_1", "$I0$r1$rX$rX")
T("integ_a_to_b", "$Ia$rb$rX$rX")
T("integ_0_to_inf", "$I0$rINF$rX$rX")
T("integ_doc", "$I0$r1/X$rABS$r$rt")
T("integ_X2", "$I0$r1$rX^2$rX")
T("integ_sin", "$I0$rPI$rX$s$rX")
T("integ_cos", "$I0$rPI$rX$c$rX")
T("integ_simple_2", "$I1$r2$rX$rX")
T("integ_simple_3", "$I0$r10$rX$rX")
T("integ_sub", "$I0$r1$r2*X$rX")
T("integ_div", "$I1$r2$r1/X$rX")
T("integ_pow", "$I0$r1$rX^2$rX")
T("integ_sqrt", "$I0$r1$r@X$rX")
T("integ_abs", "$I0$r1$rX|$rX")
T("integ_basic_t", "$I0$rT$rX$rT")
T("integ_constant", "$I0$r1$r1$rX")
T("integ_zero", "$I0$r0$rX$rX")
T("integ_neg", "$I0$r1$r-X$rX")
T("integ_sum_inside", "$I0$r1$rX+Y$rX")
T("integ_long_name", "$Ia$rb$rABS$rt")
T("integ_lo_X_hi_2X", "$IX$r2*X$rX$rX")
T("integ_F_of_t", "$I0$r1$rF$rt")
T("integ_lo_PI_hi_2PI", "$IPI$r2*PI$rX$rX")
T("integ_X_2", "$I0$rX$r2$rX")

# --- 14) sums (~15) ---
T("sum_basic", "$Sk$r1$rn$rk")
T("sum_squared", "$Sk$r1$rn$rk^2")
T("sum_cubed", "$Sk$r1$rn$rk^3")
T("sum_geom", "$Sk$r0$rn$rX^k")
T("sum_long", "$Si$r1$rN$ri")
T("sum_const", "$Sk$r1$rn$r1")
T("sum_to_inf", "$Sk$r0$rINF$rk")
T("sum_div", "$Sk$r1$rn$r1/k")
T("sum_X_k", "$Sk$r1$rn$rX*k")
T("sum_negative", "$Sk$r1$rn$r-k")
T("sum_inv_k", "$Si$r1$rn$r1/i")
T("sum_basic_two", "$Sk$r2$r5$rk")
T("sum_func", "$Sk$r1$rn$rk$s")
T("sum_polynomial", "$Sk$r0$rn$rk^2+1")
T("sum_double", "$Sk$r1$rn$rk*X")

# --- 15) derivatives (~10) ---
T("deriv_X_X", "$PX$rX")
T("deriv_X_X2", "$PX$rX^2")
T("deriv_X_sin_X", "$PX$rX$s")
T("deriv_t_X_squared", "$Pt$rX^2")
T("deriv_X_X_div_X", "$PX$rX/X")
T("deriv_X_e_x", "$PX$re^X")
T("deriv_X_const", "$PX$r1")
T("deriv_X_PI", "$PX$rPI")
T("deriv_X_X_plus_Y", "$PX$rX+Y")
T("deriv_X_X_squared_plus_1", "$PX$rX^2+1")

# --- 16) where (~10) ---
T("where_X_X_5", "X+1$WX$r5")
T("where_X2_X_2", "X^2$WX$r2")
T("where_X_PI", "X*2$WX$rPI")
T("where_a_b_c", "A+B$WA$r1")
T("where_long", "X+Y+Z$WX$r1")
T("where_div", "X/Y$WX$r2")
T("where_pow", "X^2$WX$r3")
T("where_sin", "X$s$WX$r0")
T("where_cos", "X$c$WX$r0")
T("where_const_eq", "5$WX$r5")

# --- 17) complex numbers (~10) ---
T("complex_3_4", "3$Z4")
T("complex_X_Y", "X$ZY")
T("complex_0_1", "0$Z1")
T("complex_neg_neg", "-1$Z-1")
T("complex_decimal", "1.5$Z2.5")
T("complex_PI_Y", "PI$ZY")
T("complex_a_b", "A$ZB")
T("complex_X_squared_Y", "X^2$ZY")
T("complex_neg_X_Y", "-X$ZY")
T("complex_zero_zero", "0$Z0")

# --- 18) user functions (~10) ---
T("uf_F_x", "F$FX")
T("uf_G_X_Y", "G$FX,Y")
T("uf_H_a_b_c", "H$FA,B,R")
T("uf_F_2", "F$F2")
T("uf_g_X_Y_Z", "g$FX,Y,Z")
T("uf_F_neg_X", "F$F-X")
T("uf_F_X_plus_Y", "F$FX+Y")
T("uf_F_X_X", "F$FX*X")
T("uf_F_decimal", "F$F3.14")
T("uf_F_long_arg", "F$FVARIABLE")

# --- 19) edits / deletions (~30) ---
T("edit_bksp_one", "5$b")
T("edit_bksp_two", "12$b")
T("edit_bksp_three", "123$b$b")
T("edit_replace_after_bksp", "5$bX")
T("edit_X_then_letter", "Xy")
T("edit_long_name_then_bksp", "ABCD$b")
T("edit_5_op_3", "5+3")
T("edit_arrow_left", "5+3$l")
T("edit_arrow_right", "5+3$r")
T("edit_arrow_up", "5+3$u")
T("edit_arrow_down", "5+3$k")
T("edit_two_arrows", "5+3$l$l")
T("edit_three_arrows", "5+3$l$l$l")
T("edit_inside_div", "5/3$u")
T("edit_inside_pow", "X^2$u")
T("edit_double_bksp", "12$b$b")
T("edit_clr_after", "5+3$K")
T("edit_clr_all", "5+3$L")
T("edit_two_then_replace", "12$bX")
T("edit_select_replace", "X+5$l$lY")
T("edit_5_3_two_arrows", "5*3$u")
T("edit_complex_seq", "5+X*2-3")
T("edit_replace_inside", "5+3$lY")
T("edit_arrow_through_div", "1/2$u")
T("edit_arrow_inside_sqrt", "@X$l")
T("edit_arrow_inside_paren", "X(+1$l")
T("edit_after_func", "X$s$l")
T("edit_typing_after_select", "5+3$lY")
T("edit_letter_then_bksp", "abc$b")
T("edit_simple_overwrite", "X$bY")

# --- 20) navigation (~30) ---
T("nav_simple_right", "5+3$r$r")
T("nav_simple_left", "5+3$l$l")
T("nav_up_in_div", "1/2$u$u")
T("nav_down_in_div", "1/2$k$k")
T("nav_around_pow", "X^2$u$k")
T("nav_around_sqrt", "@X$u")
T("nav_in_func", "X$s$k")
T("nav_in_func_args", "F$FX,Y$l")
T("nav_around_paren", "X+Y($u")
T("nav_through_complex", "3$Z4$l$r")
T("nav_in_integ", "$I0$rX$rX$rX$l")
T("nav_in_sum", "$Sk$r1$rn$rk$l")
T("nav_through_long_expr", "5+3*4$l")
T("nav_after_left", "5+3$l")
T("nav_after_right", "5+3$r")
T("nav_after_two_lefts", "5+3$l$l")
T("nav_after_three_rights", "5+3$r$r$r")
T("nav_in_neg", "-5$u")
T("nav_back_to_root", "X+Y+Z$u$u$u")
T("nav_first_arg", "5+3$l$l$l")
T("nav_last_arg", "5+3*4$r$r$r")
T("nav_nested_div_up", "1/X+Y$u")
T("nav_after_paren_wrap", "X+Y($u")
T("nav_in_abs", "X|$u")
T("nav_in_complex", "5$Z4$u")
T("nav_in_two_arg_func", "F$FX,Y$l")
T("nav_in_three_arg_func", "F$FX,Y,Z$l")
T("nav_after_userfunc", "F$FX,Y$r")
T("nav_in_pow_after_arrows", "X^2$u$k")
T("nav_complex_path", "1+2*3$u$u$k")

# --- 21) auto-multiply (~15) ---
T("auto_2X", "2X")
T("auto_3PI", "3PI")
T("auto_5L", "5L")
T("auto_2X_again", "2X")
T("auto_decimal", "0.5X")
T("auto_3R", "3R")
T("auto_2X_3", "2X*3")
T("auto_2L", "2L")
T("auto_4_OMEGA", "4OMEGA")
T("auto_X_then_2", "X2")
T("auto_Xy", "Xy")
T("auto_2X_plus_Y", "2X+Y")
T("auto_3X_squared", "3X^2")
T("auto_2_minus_X", "2-X")
T("auto_3PI_R", "3PI*R")

# --- 22) doc examples (~10) ---
T("doc_meta_kernel_integ", "E$I0$r1/X$rABS$r$rt")
T("doc_user_guide_arith", "5*1+1/7.5$r/@3-2^3")
T("doc_user_guide_algebra", "2*L*@1+X/R$r/R+y$r+2*L/b")
T("doc_user_guide_greek", "2/@3$rL+e^-Mu*LNX+2*Mu*Y$r/THETA^1/3")
T("doc_pythagorean", "@A^2+B^2")
T("doc_sin_cos_sq", "X$s^2+X$c^2")
T("doc_quadratic", "A*X^2+B*X+R")
T("doc_simple_geom", "PI*R^2")
T("doc_volume", "4/3*PI*R^3")
T("doc_simple_log", "X$n+Y$n")

print(f"Total tests defined: {len(TESTS)}", file=sys.stderr)
assert len(TESTS) >= 500, f"only {len(TESTS)} tests"

# ----- emit scripts -----
with open(os.path.join(SCRIPTS_DIR, "sdleqw_master.script"), "w") as f:
    f.write("# sdleqw_master.script — auto-generated\n")
    for i, (name, keys) in enumerate(TESTS):
        f.write(f"# Test {i}: {name} keys={keys!r}\n")
        f.write("RESET\n")
        f.write(f"NAME {name}\n")
        f.write(f"KEYS {keys}\n")
        f.write(f"SNAP {name}\n\n")
    f.write("QUIT\n")

with open(os.path.join(SCRIPTS_DIR, "x50ng_master.script"), "w") as f:
    f.write("# x50ng_master.script — auto-generated\n")
    f.write("WAIT 100\nSNAP boot\n\n")
    for i, (name, keys) in enumerate(TESTS):
        try:
            tap_lines = X50Builder().lower(keys)
        except Exception as e:
            print(f"!! skip {name}: {e}", file=sys.stderr)
            continue
        f.write(f"# Test {i}: {name} keys={keys!r}\n")
        f.write("TAP RIGHTSHIFT\nTAP O\nWAIT 25\n")
        for line in tap_lines:
            f.write(line + "\n")
        f.write("WAIT 20\n")
        f.write(f"SNAP {name}\n")
        f.write("TAP ON\nWAIT 15\n\n")
    f.write("QUIT\n")

# Per-chunk x50ng scripts for parallel running
N_CHUNKS = int(os.environ.get("CHUNKS", "16"))
chunk_size = (len(TESTS) + N_CHUNKS - 1) // N_CHUNKS
for ci in range(N_CHUNKS):
    chunk_tests = TESTS[ci * chunk_size : (ci + 1) * chunk_size]
    if not chunk_tests:
        continue
    path = os.path.join(SCRIPTS_DIR, f"x50ng_chunk_{ci:02d}.script")
    with open(path, "w") as f:
        f.write(f"# chunk {ci:02d} — tests {ci*chunk_size}..{ci*chunk_size+len(chunk_tests)-1}\n")
        f.write("WAIT 100\nSNAP boot\n\n")
        for j, (name, keys) in enumerate(chunk_tests):
            i = ci * chunk_size + j
            try:
                tap_lines = X50Builder().lower(keys)
            except Exception as e:
                print(f"!! skip {name}: {e}", file=sys.stderr)
                continue
            f.write(f"# Test {i}: {name} keys={keys!r}\n")
            f.write("TAP RIGHTSHIFT\nTAP O\nWAIT 25\n")
            for line in tap_lines:
                f.write(line + "\n")
            f.write("WAIT 20\n")
            f.write(f"SNAP {name}\n")
            f.write("TAP ON\nWAIT 15\n\n")
        f.write("QUIT\n")
print(f"wrote {N_CHUNKS} chunks (size ~{chunk_size})")

with open(os.path.join(SCRIPTS_DIR, "tests.csv"), "w") as f:
    f.write("idx,name,keys\n")
    for i, (name, keys) in enumerate(TESTS):
        ek = keys.replace('"', '""')
        f.write(f'{i},{name},"{ek}"\n')
