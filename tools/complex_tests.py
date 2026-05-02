#!/usr/bin/env python3
"""
complex_tests.py — render ~20 well-known complex math expressions on both
sdleqw and x50ng to verify behavioral fidelity.

Each test gives a sdleqw mini-language key sequence. We translate to x50ng
using the same X50Builder as gen_tests.py.
"""
import os, sys

# Re-use builder from gen_tests
sys.path.insert(0, os.path.dirname(__file__))
import gen_tests  # noqa: E402
X50Builder = gen_tests.X50Builder

OUT = os.path.join(os.path.dirname(__file__), "..", "scripts")
os.makedirs(OUT, exist_ok=True)

# Each entry: (name, description, sdleqw key string)
# Keys language reminder (gen_tests.py):
#   digits/letters: literal char insertion
#   + - * / : binary ops
#   ^ : power; @ : sqrt; # : nth-root
#   ( : parens wrap; | : abs wrap; , : next arg
#   $l/$r/$u/$k arrows; $b backspace
#   $I integral; $S sum; $P partial; $W where; $Z complex; $F userfunc
#   $s/$c/$T/$n/$X/$g/$a SIN/COS/TAN/LN/EXP/LOG/ABS

TESTS = [
    # Quadratic formula: x = (-b + sqrt(b^2 - 4ac)) / (2a)
    ("quad_formula",
     "Quadratic formula numerator x = (-b + sqrt(b^2-4ac)) / (2a)",
     "-b+@b^2-4ac$r$r/2a"),

    # Pythagorean theorem: c = sqrt(a^2 + b^2)
    ("pythagoras",
     "c = sqrt(a^2 + b^2)",
     "@a^2+b^2"),

    # Binomial: (x+y)^n
    ("binomial",
     "(x+y)^n",
     "x+y(^n"),

    # Euler's identity expression: e^(i*pi) + 1
    ("euler_id",
     "e^(i*pi) + 1",
     "e^i*PI$r+1"),

    # cos(2x) double angle: cos(2x) = cos(x)^2 - sin(x)^2
    ("cos_2x_identity",
     "cos(x)^2 - sin(x)^2",
     "X$c^2-X$s^2"),

    # Sum_{k=1}^{n} k^2 = n(n+1)(2n+1)/6
    ("sum_squares",
     "Sum k=1..n k^2",
     "$Sk$r1$rn$rk^2"),

    # Geometric series: sum_{k=0}^{infty} x^k = 1/(1-x)
    ("geom_series",
     "Sum_{k=0}^∞ x^k",
     "$Sk$r0$rINF$rx^k"),

    # Stirling: n! ~ sqrt(2*pi*n) * (n/e)^n
    ("stirling",
     "Stirling's approximation",
     "@2*PI*n$r*n/e$r^n"),

    # Standard normal PDF: (1/sqrt(2*pi)) * exp(-x^2/2)
    ("normal_pdf",
     "Normal PDF",
     "1/@2*PI$r*e^-x^2/2"),

    # Bayes: P(A|B) = P(B|A)*P(A) / P(B)
    ("bayes",
     "Bayes' rule",
     "P$FA$WB$r/P$FB$rP$FA$rP$FB"),

    # Definite integral of x^2 from 0 to 1
    ("def_integ_x2",
     "∫_0^1 x^2 dx",
     "$I0$r1$rx^2$rx"),

    # Indefinite integral of sin x: ∫ sin x dx
    ("indef_integ_sin",
     "∫ sin(x) dx",
     "$I$r$rx$s$rx"),

    # Limit: lim_{x->0} sin(x)/x = 1
    ("limit_sinx_x",
     "sin(x)/x",
     "x$s/x"),

    # Cauchy-Schwarz inequality form: (sum xy)^2 <= sum x^2 * sum y^2
    ("cauchy_schwarz",
     "(Σ xy)² ≤ Σx²·Σy² (LHS only)",
     "$Sk$r1$rn$rx*y$r^2"),

    # Heat equation form: ∂u/∂t = a^2 ∂²u/∂x²
    ("heat_eq",
     "∂u/∂t = a²·∂²u/∂x²",
     "$Pt$ru/$Pt$ru"),  # simplified

    # Continuous interest: A = P*(1 + r/n)^(n*t)
    ("compound_interest",
     "A = P(1 + r/n)^(nt)",
     "P*1+r/n$r$r^n*t"),

    # Determinant 2x2: ad - bc
    ("det_2x2",
     "ad - bc",
     "ad-bc"),

    # Rotation matrix entry: cos(theta)
    ("rot_cos",
     "cos(theta)",
     "THETA$c"),

    # Volume of sphere: (4/3) * pi * r^3
    ("sphere_vol",
     "(4/3)π r³",
     "4/3*PI*r^3"),

    # Kinetic energy: (1/2) m v^2
    ("kinetic_E",
     "½ m v²",
     "1/2*m*v^2"),

    # Schrodinger time-independent (operator-style): -hbar^2/(2m) * d²ψ/dx²
    ("schroedinger_kinetic",
     "−ℏ²/(2m) d²ψ/dx²",
     "-h^2/2*m$r*$Px$rPSI"),

    # Coulomb's law: F = k q1 q2 / r^2
    ("coulomb",
     "F = k q₁q₂/r²",
     "k*Q1*Q2/r^2"),

    # Newton's law of gravity: F = G m1 m2 / r^2
    ("newton_grav",
     "G m1 m2 / r²",
     "G*M1*M2/r^2"),

    # Logistic: 1 / (1 + e^(-x))
    ("sigmoid",
     "Sigmoid function",
     "1/1+e^-x"),

    # Triangle area: (1/2) base * height
    ("tri_area",
     "½ base × height",
     "1/2*b*h"),
]

print(f"complex_tests: {len(TESTS)} cases", file=sys.stderr)

# Emit sdleqw script
with open(os.path.join(OUT, "complex_sdleqw.script"), "w") as f:
    f.write("# complex_sdleqw.script — auto-generated\n")
    for name, desc, keys in TESTS:
        f.write(f"# {desc}\n")
        f.write("RESET\n")
        f.write(f"NAME {name}\n")
        f.write(f"KEYS {keys}\n")
        f.write(f"SNAP {name}\n\n")
    f.write("QUIT\n")

# Emit x50ng script (single-chunk, runs sequentially)
with open(os.path.join(OUT, "complex_x50ng.script"), "w") as f:
    f.write("# complex_x50ng.script — auto-generated\n")
    f.write("WAIT 100\nSNAP boot\n\n")
    for name, desc, keys in TESTS:
        try:
            tap_lines = X50Builder().lower(keys)
        except Exception as e:
            print(f"!! skip {name}: {e}", file=sys.stderr)
            continue
        f.write(f"# {desc}\n")
        f.write("TAP RIGHTSHIFT\nTAP O\nWAIT 25\n")
        for line in tap_lines:
            f.write(line + "\n")
        f.write("WAIT 20\n")
        f.write(f"SNAP {name}\n")
        f.write("TAP ON\nWAIT 15\n\n")
    f.write("QUIT\n")

# Also csv
with open(os.path.join(OUT, "complex_tests.csv"), "w") as f:
    f.write("idx,name,desc,keys\n")
    for i, (name, desc, keys) in enumerate(TESTS):
        d = desc.replace('"', '""')
        k = keys.replace('"', '""')
        f.write(f'{i},{name},"{d}","{k}"\n')

print(f"wrote scripts to {OUT}/", file=sys.stderr)
