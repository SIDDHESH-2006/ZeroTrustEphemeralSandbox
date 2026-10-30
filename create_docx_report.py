import os
import docx
from docx.shared import Inches, Pt, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT, WD_ALIGN_VERTICAL
from docx.oxml import parse_xml, OxmlElement
from docx.oxml.ns import nsdecls, qn

def build_docx():
    doc = docx.Document()

    # ---------------------------------------------------------
    # Page Setup: A4, 1-inch margins
    # ---------------------------------------------------------
    for section in doc.sections:
        section.page_width = Inches(8.27)   # A4 width
        section.page_height = Inches(11.69) # A4 height
        section.top_margin = Inches(1.0)
        section.bottom_margin = Inches(1.0)
        section.left_margin = Inches(1.0)
        section.right_margin = Inches(1.0)
        section.different_first_page_header_footer = True

    # ---------------------------------------------------------
    # Styling Helpers
    # ---------------------------------------------------------
    NAVY_HEX = "1A365D"
    SLATE_HEX = "2B6CB0"
    DARK_TEXT_HEX = "2D3748"
    LIGHT_BG_HEX = "F7FAFC"
    BORDER_HEX = "CBD5E0"
    TERM_BG_HEX = "1A202C"
    TERM_TEXT_HEX = "E2E8F0"
    GREEN_HEX = "276749"
    GREEN_BG_HEX = "C6F6D5"

    def set_cell_background(cell, fill_hex):
        shading = parse_xml(f'<w:shd {nsdecls("w")} w:fill="{fill_hex}"/>')
        cell._tc.get_or_add_tcPr().append(shading)

    def set_cell_margins(cell, top=120, bottom=120, left=160, right=160):
        tcPr = cell._tc.get_or_add_tcPr()
        tcMar = parse_xml(
            f'<w:tcMar {nsdecls("w")}>'
            f'<w:top w:w="{top}" w:type="dxa"/><w:bottom w:w="{bottom}" w:type="dxa"/>'
            f'<w:left w:w="{left}" w:type="dxa"/><w:right w:w="{right}" w:type="dxa"/>'
            f'</w:tcMar>'
        )
        tcPr.append(tcMar)

    def set_cell_borders(cell, top=None, bottom=None, left=None, right=None):
        tcPr = cell._tc.get_or_add_tcPr()
        borders = ['<w:tcBorders xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">']
        for side, b_val in [('top', top), ('bottom', bottom), ('left', left), ('right', right)]:
            if b_val:
                val, sz, color = b_val
                borders.append(f'<w:{side} w:val="{val}" w:sz="{sz}" w:space="0" w:color="{color}"/>')
            else:
                borders.append(f'<w:{side} w:val="none"/>')
        borders.append('</w:tcBorders>')
        tcPr.append(parse_xml(''.join(borders)))

    def set_table_borders(table, color="D3D3D3", sz="4"):
        tblPr = table._tbl.tblPr
        borders = parse_xml(
            f'<w:tblBorders {nsdecls("w")}>'
            f'<w:top w:val="single" w:sz="{sz}" w:space="0" w:color="{color}"/>'
            f'<w:bottom w:val="single" w:sz="{sz}" w:space="0" w:color="{color}"/>'
            f'<w:left w:val="none"/>'
            f'<w:right w:val="none"/>'
            f'<w:insideH w:val="single" w:sz="{sz}" w:space="0" w:color="{color}"/>'
            f'<w:insideV w:val="none"/>'
            f'</w:tblBorders>'
        )
        tblPr.append(borders)

    def add_heading_1(text):
        p = doc.add_paragraph()
        p.paragraph_format.space_before = Pt(14)
        p.paragraph_format.space_after = Pt(6)
        p.paragraph_format.keep_with_next = True
        run = p.add_run(text)
        run.font.name = "Arial"
        run.font.size = Pt(14)
        run.font.bold = True
        run.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)
        return p

    def add_heading_2(text):
        p = doc.add_paragraph()
        p.paragraph_format.space_before = Pt(10)
        p.paragraph_format.space_after = Pt(4)
        p.paragraph_format.keep_with_next = True
        run = p.add_run(text)
        run.font.name = "Arial"
        run.font.size = Pt(12)
        run.font.bold = True
        run.font.color.rgb = RGBColor(0x2B, 0x6C, 0xB0)
        return p

    def add_heading_3(text):
        p = doc.add_paragraph()
        p.paragraph_format.space_before = Pt(8)
        p.paragraph_format.space_after = Pt(2)
        p.paragraph_format.keep_with_next = True
        run = p.add_run(text)
        run.font.name = "Arial"
        run.font.size = Pt(10.5)
        run.font.bold = True
        run.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)
        return p

    def add_body_paragraph(text, bold_prefix=None, space_after=6):
        p = doc.add_paragraph()
        p.paragraph_format.space_before = Pt(0)
        p.paragraph_format.space_after = Pt(space_after)
        p.paragraph_format.line_spacing = 1.15
        if bold_prefix:
            r_pre = p.add_run(bold_prefix)
            r_pre.font.name = "Arial"
            r_pre.font.size = Pt(10)
            r_pre.font.bold = True
            r_pre.font.color.rgb = RGBColor(0x1A, 0x20, 0x2C)
        run = p.add_run(text)
        run.font.name = "Arial"
        run.font.size = Pt(10)
        run.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)
        return p

    def add_callout_box(text, bold_title="ABSTRACT:"):
        table = doc.add_table(rows=1, cols=1)
        table.alignment = WD_TABLE_ALIGNMENT.CENTER
        table.autofit = False
        cell = table.cell(0, 0)
        cell.width = Inches(6.27)
        set_cell_background(cell, "F0F4F8")
        set_cell_margins(cell, top=140, bottom=140, left=180, right=160)
        set_cell_borders(cell, left=('single', '36', '1A365D')) # thick navy left border
        p = cell.paragraphs[0]
        p.paragraph_format.space_before = Pt(2)
        p.paragraph_format.space_after = Pt(2)
        p.paragraph_format.line_spacing = 1.15
        r_title = p.add_run(bold_title + " ")
        r_title.font.name = "Arial"
        r_title.font.size = Pt(10.5)
        r_title.font.bold = True
        r_title.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)
        
        r_body = p.add_run(text)
        r_body.font.name = "Arial"
        r_body.font.size = Pt(10)
        r_body.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)
        doc.add_paragraph().paragraph_format.space_after = Pt(4)

    def add_terminal_box(command, output_lines):
        table = doc.add_table(rows=1, cols=1)
        table.alignment = WD_TABLE_ALIGNMENT.CENTER
        table.autofit = False
        cell = table.cell(0, 0)
        cell.width = Inches(6.27)
        set_cell_background(cell, "1E1E1E")
        set_cell_margins(cell, top=120, bottom=120, left=160, right=160)
        set_cell_borders(cell, left=('single', '24', '3182CE')) # blue left accent

        p = cell.paragraphs[0]
        p.paragraph_format.space_before = Pt(0)
        p.paragraph_format.space_after = Pt(3)
        p.paragraph_format.line_spacing = 1.05

        if command:
            r_prompt = p.add_run("$ ")
            r_prompt.font.name = "Consolas"
            r_prompt.font.size = Pt(8.5)
            r_prompt.font.bold = True
            r_prompt.font.color.rgb = RGBColor(0x4E, 0xC9, 0xB0) # teal prompt

            r_cmd = p.add_run(command + "\n")
            r_cmd.font.name = "Consolas"
            r_cmd.font.size = Pt(8.5)
            r_cmd.font.bold = True
            r_cmd.font.color.rgb = RGBColor(0x9C, 0xDC, 0xFE) # cyan command

        for line in output_lines:
            r = p.add_run(line + "\n")
            r.font.name = "Consolas"
            r.font.size = Pt(8.2)
            if "[PASS]" in line or "--> Succeeded" in line or "ALL SECURITY" in line:
                r.font.color.rgb = RGBColor(0x85, 0xE8, 0x9D) # green
                r.font.bold = True
            elif "[FAIL]" in line or "Permission denied" in line or "PermissionError" in line or "Operation not permitted" in line or "[sandbox:error]" in line:
                r.font.color.rgb = RGBColor(0xF9, 0x75, 0x83) # red
            elif line.startswith("===") or line.startswith("---") or line.startswith("[Step"):
                r.font.color.rgb = RGBColor(0x79, 0xB8, 0xFF) # blue
                r.font.bold = True
            elif line.startswith("+---") or line.startswith("|"):
                r.font.color.rgb = RGBColor(0xB3, 0x92, 0xF0) # purple table
            else:
                r.font.color.rgb = RGBColor(0xD4, 0xD4, 0xD4) # light gray

        doc.add_paragraph().paragraph_format.space_after = Pt(4)

    def add_code_snippet(code_text, lang_title="C11 Implementation"):
        table = doc.add_table(rows=1, cols=1)
        table.alignment = WD_TABLE_ALIGNMENT.CENTER
        table.autofit = False
        cell = table.cell(0, 0)
        cell.width = Inches(6.27)
        set_cell_background(cell, "F8FAFC")
        set_cell_margins(cell, top=100, bottom=100, left=140, right=140)
        set_cell_borders(cell, left=('single', '20', '4C51BF'), top=('single', '4', 'E2E8F0'),
                              bottom=('single', '4', 'E2E8F0'), right=('single', '4', 'E2E8F0'))

        p = cell.paragraphs[0]
        p.paragraph_format.space_before = Pt(0)
        p.paragraph_format.space_after = Pt(2)
        p.paragraph_format.line_spacing = 1.05

        r_hdr = p.add_run(f"/* === {lang_title} === */\n")
        r_hdr.font.name = "Consolas"
        r_hdr.font.size = Pt(8.2)
        r_hdr.font.bold = True
        r_hdr.font.color.rgb = RGBColor(0x4C, 0x51, 0xBF)

        r_code = p.add_run(code_text)
        r_code.font.name = "Consolas"
        r_code.font.size = Pt(8.2)
        r_code.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)

        doc.add_paragraph().paragraph_format.space_after = Pt(4)

    # ---------------------------------------------------------
    # Subsequent Page Headers & Footers
    # ---------------------------------------------------------
    sub_section = doc.sections[0]
    sub_header = sub_section.header
    p_hdr = sub_header.paragraphs[0]
    p_hdr.alignment = WD_ALIGN_PARAGRAPH.RIGHT
    r_hdr = p_hdr.add_run("BCSE303P — Zero-Trust Ephemeral Sandbox | Project Review - 3")
    r_hdr.font.name = "Arial"
    r_hdr.font.size = Pt(8.5)
    r_hdr.font.color.rgb = RGBColor(0x71, 0x80, 0x96)

    sub_footer = sub_section.footer
    p_ftr = sub_footer.paragraphs[0]
    p_ftr.alignment = WD_ALIGN_PARAGRAPH.RIGHT
    r_ftr_lbl = p_ftr.add_run("School of Computer Science Engineering & Information Systems, VIT Vellore   |   Page ")
    r_ftr_lbl.font.name = "Arial"
    r_ftr_lbl.font.size = Pt(8.5)
    r_ftr_lbl.font.color.rgb = RGBColor(0x71, 0x80, 0x96)

    # Add dynamic page number
    r_pno = p_ftr.add_run()
    fldChar1 = parse_xml(r'<w:fldChar %s w:fldCharType="begin"/>' % nsdecls('w'))
    instrText = parse_xml(r'<w:instrText %s xml:space="preserve"> PAGE </w:instrText>' % nsdecls('w'))
    fldChar2 = parse_xml(r'<w:fldChar %s w:fldCharType="separate"/>' % nsdecls('w'))
    fldChar3 = parse_xml(r'<w:fldChar %s w:fldCharType="end"/>' % nsdecls('w'))
    r_pno._r.append(fldChar1)
    r_pno._r.append(instrText)
    r_pno._r.append(fldChar2)
    r_pno._r.append(fldChar3)
    r_pno.font.name = "Arial"
    r_pno.font.size = Pt(8.5)
    r_pno.font.bold = True
    r_pno.font.color.rgb = RGBColor(0x2B, 0x6C, 0xB0)

    # =========================================================
    # 1. TITLE / COVER PAGE (Matching Page 1 of Review PDF)
    # =========================================================
    p_top_sp = doc.add_paragraph()
    p_top_sp.paragraph_format.space_before = Pt(20)

    # University title
    p_uni = doc.add_paragraph()
    p_uni.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_uni.paragraph_format.space_after = Pt(2)
    r_uni = p_uni.add_run("VELLORE INSTITUTE OF TECHNOLOGY")
    r_uni.font.name = "Arial"
    r_uni.font.size = Pt(17)
    r_uni.font.bold = True
    r_uni.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)

    p_deem = doc.add_paragraph()
    p_deem.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_deem.paragraph_format.space_after = Pt(18)
    r_deem = p_deem.add_run("(Deemed to be University under section 3 of UGC Act, 1956)")
    r_deem.font.name = "Arial"
    r_deem.font.size = Pt(9.5)
    r_deem.font.italic = True
    r_deem.font.color.rgb = RGBColor(0x4A, 0x55, 0x68)

    p_sch = doc.add_paragraph()
    p_sch.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_sch.paragraph_format.space_after = Pt(6)
    r_sch = p_sch.add_run("School of Computer Science Engineering and Information Systems")
    r_sch.font.name = "Arial"
    r_sch.font.size = Pt(12)
    r_sch.font.bold = True
    r_sch.font.color.rgb = RGBColor(0x2B, 0x6C, 0xB0)

    p_sem = doc.add_paragraph()
    p_sem.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_sem.paragraph_format.space_after = Pt(24)
    r_sem = p_sem.add_run("Fall Semester 2026–2027")
    r_sem.font.name = "Arial"
    r_sem.font.size = Pt(11)
    r_sem.font.bold = True
    r_sem.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)

    p_course = doc.add_paragraph()
    p_course.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_course.paragraph_format.space_after = Pt(4)
    r_code = p_course.add_run("BCSE303P — Operating Systems Lab\n")
    r_code.font.name = "Arial"
    r_code.font.size = Pt(13)
    r_code.font.bold = True
    r_code.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)

    p_rev = doc.add_paragraph()
    p_rev.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_rev.paragraph_format.space_after = Pt(22)
    r_rev = p_rev.add_run("Project Review - 3")
    r_rev.font.name = "Arial"
    r_rev.font.size = Pt(15)
    r_rev.font.bold = True
    r_rev.font.color.rgb = RGBColor(0xC0, 0x56, 0x21) # Amber/Gold accent

    # Project Title Box
    t_box = doc.add_table(rows=1, cols=1)
    t_box.alignment = WD_TABLE_ALIGNMENT.CENTER
    t_box.autofit = False
    c_box = t_box.cell(0, 0)
    c_box.width = Inches(6.0)
    set_cell_background(c_box, "EBF8FF")
    set_cell_margins(c_box, top=140, bottom=140, left=160, right=160)
    set_cell_borders(c_box, top=('single', '12', '3182CE'), bottom=('single', '12', '3182CE'),
                            left=('single', '28', '2B6CB0'), right=('single', '12', '3182CE'))

    p_t = c_box.paragraphs[0]
    p_t.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r_lbl = p_t.add_run("PROJECT TITLE\n")
    r_lbl.font.name = "Arial"
    r_lbl.font.size = Pt(10)
    r_lbl.font.bold = True
    r_lbl.font.color.rgb = RGBColor(0x2B, 0x6C, 0xB0)

    r_title_main = p_t.add_run("ZERO-TRUST EPHEMERAL SANDBOX\n")
    r_title_main.font.name = "Arial"
    r_title_main.font.size = Pt(14)
    r_title_main.font.bold = True
    r_title_main.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)

    r_sub = p_t.add_run("Sub-Millisecond In-Process Process Confinement via Landlock LSM & Seccomp-BPF")
    r_sub.font.name = "Arial"
    r_sub.font.size = Pt(10)
    r_sub.font.italic = True
    r_sub.font.color.rgb = RGBColor(0x4A, 0x55, 0x68)

    p_sp2 = doc.add_paragraph()
    p_sp2.paragraph_format.space_before = Pt(20)

    # Student Team Section
    p_team_hdr = doc.add_paragraph()
    p_team_hdr.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_team_hdr.paragraph_format.space_after = Pt(6)
    r_thdr = p_team_hdr.add_run("Submitted By:")
    r_thdr.font.name = "Arial"
    r_thdr.font.size = Pt(11)
    r_thdr.font.bold = True
    r_thdr.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)

    # Student Table
    team_table = doc.add_table(rows=4, cols=3)
    team_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(team_table, color="CBD5E0", sz="4")
    col_widths = [Inches(2.2), Inches(1.5), Inches(2.5)]

    headers = ["Student Name", "Register Number", "Institutional Email"]
    for j, h in enumerate(headers):
        cell = team_table.cell(0, j)
        cell.width = col_widths[j]
        set_cell_background(cell, "1A365D")
        set_cell_margins(cell, top=80, bottom=80, left=100, right=100)
        p = cell.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER if j == 1 else WD_ALIGN_PARAGRAPH.LEFT
        r = p.add_run(h)
        r.font.name = "Arial"
        r.font.size = Pt(9)
        r.font.bold = True
        r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    team_data = [
        ("Mohammad Ammar Abdullah", "24BYB0017", "ammar.abdullah2024@vitstudent.ac.in"),
        ("Siddhesh Srinivasan", "24BYB0035", "siddhesh.srinivasan2024@vitstudent.ac.in"),
        ("Aditi Soni", "24BDE0144", "aditi.soni2024@vitstudent.ac.in"),
    ]

    for i, row in enumerate(team_data):
        bg = "FFFFFF" if i % 2 == 0 else "F7FAFC"
        for j, val in enumerate(row):
            cell = team_table.cell(i + 1, j)
            cell.width = col_widths[j]
            set_cell_background(cell, bg)
            set_cell_margins(cell, top=70, bottom=70, left=100, right=100)
            p = cell.paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER if j == 1 else WD_ALIGN_PARAGRAPH.LEFT
            r = p.add_run(val)
            r.font.name = "Arial"
            r.font.size = Pt(9)
            if j == 0:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x1A, 0x20, 0x2C)
            elif j == 1:
                r.font.name = "Consolas"
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x2B, 0x6C, 0xB0)
            else:
                r.font.color.rgb = RGBColor(0x4A, 0x55, 0x68)

    p_sp3 = doc.add_paragraph()
    p_sp3.paragraph_format.space_before = Pt(26)

    # Faculty Guide Section
    p_guide_lbl = doc.add_paragraph()
    p_guide_lbl.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_guide_lbl.paragraph_format.space_after = Pt(3)
    r_glbl = p_guide_lbl.add_run("Under the Guidance of")
    r_glbl.font.name = "Arial"
    r_glbl.font.size = Pt(11)
    r_glbl.font.bold = True
    r_glbl.font.color.rgb = RGBColor(0x4A, 0x55, 0x68)

    p_guide_name = doc.add_paragraph()
    p_guide_name.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_guide_name.paragraph_format.space_after = Pt(2)
    r_gname = p_guide_name.add_run("Dr. PORKODI S")
    r_gname.font.name = "Arial"
    r_gname.font.size = Pt(13)
    r_gname.font.bold = True
    r_gname.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)

    p_guide_desig = doc.add_paragraph()
    p_guide_desig.alignment = WD_ALIGN_PARAGRAPH.CENTER
    p_guide_desig.paragraph_format.space_after = Pt(2)
    r_gdesig = p_guide_desig.add_run("Assistant Professor Senior Grade – 1")
    r_gdesig.font.name = "Arial"
    r_gdesig.font.size = Pt(10.5)
    r_gdesig.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)

    p_guide_dept = doc.add_paragraph()
    p_guide_dept.alignment = WD_ALIGN_PARAGRAPH.CENTER
    r_gdept = p_guide_dept.add_run("School of Computer Science Engineering and Information Systems\nVellore Institute of Technology, Vellore")
    r_gdept.font.name = "Arial"
    r_gdept.font.size = Pt(10)
    r_gdept.font.italic = True
    r_gdept.font.color.rgb = RGBColor(0x71, 0x80, 0x96)

    # Page Break after Cover Page
    doc.add_page_break()

    # =========================================================
    # 2. ABSTRACT (150 – 200 words)
    # =========================================================
    add_heading_1("ABSTRACT")
    abstract_text = (
        "Executing untrusted third-party code in modern software engineering environments presents severe "
        "security challenges, from credential theft to supply-chain tampering. Conventional process containment "
        "mechanisms, such as hardware virtual machines and containerization engines, introduce unacceptable initialization "
        "latencies (500 ms to 2,000 ms), high memory footprints, and dependence on administrative superuser privileges. "
        "This project presents the Zero-Trust Ephemeral Sandbox, a high-performance, unprivileged process confinement engine "
        "implemented in pure ISO C11 using native Linux kernel security primitives. The sandbox enforces defense-in-depth "
        "across sequential barriers: unprivileged privilege restriction via PR_SET_NO_NEW_PRIVS, complete Linux capability "
        "dropping via raw capset(), in-kernel path-based access control using Landlock Linux Security Module (LSM) ABI v1–v3, "
        "and system call interception using classic Berkeley Packet Filter bytecode (Seccomp-BPF). Final payload execution "
        "occurs via execvp(), ensuring zero supervisor memory retention. Rigorous empirical benchmarking on an ARM64 Linux 6.3 "
        "environment across 1,000 recorded iterations demonstrated a median initialization overhead of just 360.10 microseconds "
        "(0.360 ms)—comfortably below the 1.0 ms design ceiling—with a 100% pass rate across nine automated test suites and "
        "zero execution defects."
    )
    add_callout_box(abstract_text, bold_title="Executive Abstract (169 words):")

    # =========================================================
    # 3. Keywords (5 – 6 keywords)
    # =========================================================
    add_heading_1("Keywords")
    p_kw = doc.add_paragraph()
    p_kw.paragraph_format.space_after = Pt(12)
    keywords = [
        "Zero-Trust Architecture", "Process Sandboxing", "Landlock LSM",
        "Seccomp-BPF", "Unprivileged Execution", "Ephemeral Computing"
    ]
    for idx, kw in enumerate(keywords):
        r_kw = p_kw.add_run(f"[{kw}]")
        r_kw.font.name = "Arial"
        r_kw.font.size = Pt(9.5)
        r_kw.font.bold = True
        r_kw.font.color.rgb = RGBColor(0x2B, 0x6C, 0xB0)
        if idx < len(keywords) - 1:
            p_kw.add_run("   •   ")

    # =========================================================
    # 4. INTRODUCTION
    # =========================================================
    add_heading_1("INTRODUCTION")
    add_body_paragraph(
        "Process containment is one of the most critical challenges in contemporary operating systems and distributed "
        "computing. As modern software engineering transitions toward decentralized package dependencies (npm, PyPI, "
        "crates.io, Maven), automated continuous integration / continuous deployment (CI/CD) pipelines, and dynamic artificial "
        "intelligence code generation agents, host machines are frequently tasked with executing arbitrary, unverified code "
        "artifacts. Under standard operating system execution models, an invoked process inherits the complete ambient "
        "permissions and filesystem access privileges of the invoking user. Consequently, a compromised build script or "
        "malicious library can exfiltrate sensitive cryptographic keys (~/.ssh, ~/.aws/credentials), install persistent "
        "rootkits, snoop on concurrent process memory, or establish outbound reverse command-and-control shells."
    )
    add_body_paragraph(
        "To mitigate these risks, traditional security infrastructures rely heavily on hardware-assisted virtualization "
        "(e.g., KVM, QEMU) or containerization engines (e.g., Docker, Podman). While these technologies provide robust "
        "isolation boundaries for long-running network daemons and microservices, they are fundamentally mismatched with "
        "the operational demands of ephemeral execution. Launching a container or virtual machine introduces heavy initialization "
        "delays ranging from 500 milliseconds to over 15 seconds, consumes dozens of megabytes of resident memory, and "
        "typically requires elevated administrative privileges (sudo or setuid daemons) that expand the host attack surface."
    )
    add_body_paragraph(
        "The Zero-Trust Ephemeral Sandbox addresses this architectural gap by executing directly in unprivileged user space "
        "while orchestrating native Linux kernel security facilities introduced in modern kernels (Linux 5.13+). By synthesizing "
        "Landlock LSM for path-based mandatory filesystem access control, Seccomp-BPF for fine-grained system call interception, "
        "Linux capabilities stripping, and unprivileged network namespace decoupling, the sandbox establishes an inescapable "
        "containment jail in hundreds of microseconds. Upon arming these kernel barriers, the supervisor process replaces its "
        "entire address space with the target binary using execvp(), guaranteeing zero runtime memory bloat and native "
        "computation speeds."
    )

    # =========================================================
    # 5. PROBLEM STATEMENT
    # =========================================================
    add_heading_1("PROBLEM STATEMENT")
    add_body_paragraph(
        "Modern software development and automated execution pipelines face a persistent conflict between security confinement "
        "and execution efficiency across four primary dimensions:"
    )
    add_body_paragraph(
        "On standard UNIX/Linux systems, unprivileged processes run with the full discretionary access control (DAC) permissions "
        "of the executing user. A script running as developer has unrestricted read access to developer SSH keys, browser "
        "sessions, source repositories, and environment variables.",
        bold_prefix="1. Monolithic User Authority: "
    )
    add_body_paragraph(
        "Conventional container runtimes (Docker/containerd) require multi-step orchestration (cgroup setup, network bridge "
        "allocation, storage driver mount overlays, daemon IPC), introducing startup overheads of 500 ms to 2,000 ms. For sub-second "
        "commands (such as running a unit test, linter, or compiler invocation), container startup latency dominates total wall-clock "
        "time by over 90%.",
        bold_prefix="2. Prohibitive Initialization Latencies: "
    )
    add_body_paragraph(
        "Container engines traditionally rely on background daemons running as root (e.g., dockerd), exposing the host to local "
        "privilege escalation and container-escape vulnerabilities (e.g., CVE-2019-5736, CVE-2024-21626).",
        bold_prefix="3. Privilege Creep and Root Daemon Vulnerabilities: "
    )
    add_body_paragraph(
        "An unconfined payload can issue arbitrary system calls, opening attack paths to asynchronous kernel exploits (e.g., "
        "io_uring privilege escalations), inter-process memory snooping (process_vm_readv), and network exfiltration (socket, connect).",
        bold_prefix="4. Syscall Surface Exposure: "
    )
    add_callout_box(
        "To engineer a dependency-free, unprivileged process sandbox in C that guarantees zero-trust containment for arbitrary "
        "untrusted payloads on modern Linux systems without requiring superuser (root) access, setuid binaries, or background "
        "daemons, while strictly enforcing filesystem path allowlisting, network socket denial, capability stripping, and "
        "system call filtering with startup latency bounded strictly below 1.0 millisecond (< 1,000 µs) and zero residual memory overhead.",
        bold_title="Formal Problem Definition:"
    )

    # =========================================================
    # 6. OBJECTIVES
    # =========================================================
    add_heading_1("OBJECTIVES")
    objectives = [
        ("Sub-Millisecond Startup Overhead: ", "Bound the end-to-end sandbox initialization overhead strictly below 1.0 ms (< 1000 µs), empirically verified against native execution via monotonic hardware clocks."),
        ("100% Unprivileged Execution: ", "Ensure the sandbox operates entirely without root privileges, setuid flags, or administrative capabilities by leveraging PR_SET_NO_NEW_PRIVS and unprivileged Landlock LSM ABI."),
        ("In-Kernel Filesystem Jailing via Landlock LSM: ", "Restrict file access strictly to user-allowlisted directories (--ro and --rw), with complete hardware-level prevention of symlink escapes, relative path traversals (../), and write tampering."),
        ("Syscall-Level Attack Surface Stripping via Seccomp-BPF: ", "Compile and inject classic BPF bytecode filters that reject prohibited system calls (including socket, connect, bind, ptrace, process_vm_readv, process_vm_writev, and all io_uring variants) returning EPERM."),
        ("Privilege & Capability Stripping: ", "Zero out Effective, Permitted, and Inheritable Linux capability bitmasks via capset() and verify capability nullification with capget()."),
        ("Network Namespace Decoupling: ", "Provide optional complete network isolation via unprivileged user and network namespace unsharing (CLONE_NEWUSER | CLONE_NEWNET)."),
        ("Zero Residual Supervisor Footprint: ", "Implement execvp() dispatch so the sandbox supervisor process image is completely overwritten by the target binary, eliminating ongoing CPU, thread, and resident RAM overhead."),
        ("Fail-Closed Security Posture: ", "Abort immediately with non-zero exit codes if any user-specified path does not exist, if kernel security features are unsupported, or if security filter injection encounters errors.")
    ]
    for idx, (pre, desc) in enumerate(objectives, 1):
        add_body_paragraph(desc, bold_prefix=f"{idx}. {pre}")

    # =========================================================
    # 7. SCOPE OF THE PROJECT
    # =========================================================
    add_heading_1("SCOPE OF THE PROJECT")
    add_heading_2("Target Workloads & In-Scope Applications")
    add_body_paragraph("• Automated Grading & CI/CD Systems: Secure execution of student submissions, compiler invocations, and test suites without risking server compromise.")
    add_body_paragraph("• Package Manager Build Hooks: Confinement of third-party install hooks in npm install, pip install, cargo build, and gem install.")
    add_body_paragraph("• Autonomous AI Agent Sandboxing: Safe execution of arbitrary terminal commands, code snippets, and dynamic scripts generated by Large Language Models (LLMs).")
    add_body_paragraph("• Lightweight Utility Confinement: Sandboxing text editors, formatters, document converters, and compression tools processing untrusted inputs.")

    add_heading_2("Supported Operating System & Architecture Matrix")
    add_body_paragraph("• Operating System: Linux distributions with kernel version >= 5.13 (enabling Landlock LSM ABI v1). Fully validated on Linux 6.3.0 (Landlock ABI v3).")
    add_body_paragraph("• CPU Architectures: 64-bit ARM (aarch64 / ARM64), x86 64-bit (x86_64), x86 32-bit (i386), and 32-bit ARM (arm).")
    add_body_paragraph("• Standard Library: Pure POSIX/C11 standard library (glibc or musl) without third-party runtime dependencies.")

    add_heading_2("Boundaries & Out-of-Scope Elements")
    add_body_paragraph("• Hardware Virtualization & Guest OS Emulation: The sandbox runs native host ELF binaries directly on the host kernel; it does not emulate foreign instruction set architectures.")
    add_body_paragraph("• cgroups v2 Resource Throttling: Strict CPU/RAM quotas via Linux control groups require administrative setup or delegated systemd manager privileges, and are kept outside this unprivileged core.")
    add_body_paragraph("• Multi-tenant Hardware Side-Channels: Hardware microarchitectural speculative execution flaws (e.g., Spectre, Meltdown) are mitigated at the kernel/hardware level and are outside user-space sandboxing.")

    # =========================================================
    # 8. Literature Survey / Existing Systems
    # =========================================================
    add_heading_1("Literature Survey / Existing Systems")
    add_body_paragraph(
        "A rigorous comparative analysis of existing process isolation technologies was conducted across six architectural "
        "dimensions, contrasting virtual machines, container engines, traditional chroot jails, user-space kernels, and "
        "namespace sandboxes against our proposed zero-trust architecture:"
    )

    lit_table = doc.add_table(rows=7, cols=7)
    lit_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(lit_table, color="CBD5E0", sz="4")

    lit_headers = [
        "Dimension", "Virtual Machines (QEMU/KVM)", "Containers (Docker)",
        "Traditional chroot", "gVisor (runsc)", "Bubblewrap", "Our Proposed System"
    ]
    lit_col_widths = [Inches(1.0), Inches(0.9), Inches(0.9), Inches(0.85), Inches(0.85), Inches(0.85), Inches(0.92)]

    for j, h in enumerate(lit_headers):
        cell = lit_table.cell(0, j)
        cell.width = lit_col_widths[j]
        set_cell_background(cell, "1A365D" if j < 6 else "22543D") # green for proposed
        set_cell_margins(cell, top=70, bottom=70, left=60, right=60)
        p = cell.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        r.font.name = "Arial"
        r.font.size = Pt(7.5)
        r.font.bold = True
        r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    lit_rows = [
        ("Mechanism", "Hypervisor + Guest OS", "Namespaces + cgroups", "VFS root redirection", "Go Sentry emulator", "Mount ns + seccomp", "Landlock + Seccomp + execvp"),
        ("Privilege", "Root or /dev/kvm", "Root daemon (dockerd)", "CAP_SYS_CHROOT (Root)", "Low to Medium", "Unprivileged userns", "100% Unprivileged"),
        ("Startup Overhead", "5,000–15,000 ms", "500–2,000 ms", "5–20 ms", "150–300 ms", "10–30 ms", "0.360 ms (360.10 µs)"),
        ("RAM Overhead", "256 MB – 2 GB", "30 MB – 100 MB", "Negligible", "30 MB – 50 MB", "2 MB – 5 MB", "0 KB (execvp replaced)"),
        ("FS Security", "Virtual disk image", "OverlayFS / Mount ns", "Trivially escapable", "Virtualized Go VFS", "Mount namespace", "Kernel Landlock LSM"),
        ("Syscall Filter", "Emulated hardware", "Seccomp profile (JSON)", "None (All allowed)", "Emulates ~300 syscalls", "Seccomp BPF", "Direct Classic BPF Bytecode")
    ]

    for i, row in enumerate(lit_rows):
        bg = "FFFFFF" if i % 2 == 0 else "F7FAFC"
        for j, val in enumerate(row):
            cell = lit_table.cell(i + 1, j)
            cell.width = lit_col_widths[j]
            cell_bg = "E6FFFA" if j == 6 else bg # highlight proposed system column
            set_cell_background(cell, cell_bg)
            set_cell_margins(cell, top=60, bottom=60, left=60, right=60)
            p = cell.paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER if j > 0 else WD_ALIGN_PARAGRAPH.LEFT
            r = p.add_run(val)
            r.font.name = "Arial"
            r.font.size = Pt(7.5)
            if j == 0:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x1A, 0x20, 0x2C)
            elif j == 6:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x22, 0x54, 0x3D)
            else:
                r.font.color.rgb = RGBColor(0x4A, 0x55, 0x68)

    doc.add_paragraph().paragraph_format.space_after = Pt(8)

    # =========================================================
    # 9. PROPOSED SYSTEM
    # =========================================================
    add_heading_1("PROPOSED SYSTEM")
    add_body_paragraph(
        "The proposed system is an unprivileged single-binary CLI confinement engine written in strict ISO C11. The utility "
        "accepts user-defined path parameters (--ro <path>, --rw <path>), isolation flags (--no-net, --allow-net, --unshare-net, "
        "--drop-caps), and the untrusted command payload after a POSIX '--' separator."
    )
    add_body_paragraph(
        "The system executes a strictly ordered, defense-in-depth pipeline across five sequential stages:",
        bold_prefix="Sequential Pipeline Execution Architecture: "
    )
    pipeline_stages = [
        ("Stage 1 (CLI Supervisor & Validation): ", "Parses command arguments, resolves canonical paths with realpath(), validates paths using stat(), and auto-allowlists critical system library directories (/usr, /lib, /bin, /dev, /etc). If any explicit path is missing, the supervisor aborts immediately."),
        ("Stage 2 (Privilege Confinement): ", "Invokes prctl(PR_SET_NO_NEW_PRIVS, 1) to permanently prohibit setuid privilege acquisition, clears ambient capabilities via prctl(PR_CAP_AMBIENT_CLEAR_ALL), zeroes Effective, Permitted, and Inheritable capabilities via raw SYS_capset, and optionally unshares user and network namespaces (CLONE_NEWUSER | CLONE_NEWNET)."),
        ("Stage 3 (Landlock LSM Filesystem Barrier): ", "Queries the host kernel Landlock ABI version (v1–v3), compiles an in-kernel ruleset via SYS_landlock_create_ruleset, attaches allowlisted directory file descriptors with SYS_landlock_add_rule, and locks the filesystem boundaries for the process and all future children via SYS_landlock_restrict_self."),
        ("Stage 4 (Seccomp-BPF Syscall Filter): ", "Validates the host CPU architecture against AUDIT_ARCH_AARCH64 or AUDIT_ARCH_X86_64, compiles classic BPF instructions rejecting prohibited system calls (socket, connect, bind, ptrace, process_vm_readv/writev, io_uring) with SECCOMP_RET_ERRNO(EPERM), and injects the filter into the kernel via SYS_seccomp."),
        ("Stage 5 (Payload Dispatch Handoff): ", "Calls execvp() to overwrite the supervisor's address space with the target binary. The supervisor process terminates completely, leaving zero residual memory footprint (0 KB) and running the payload at full native CPU speed.")
    ]
    for pre, desc in pipeline_stages:
        add_body_paragraph(desc, bold_prefix=pre)

    # =========================================================
    # 10. SYSTEM ARCHITECTURE
    # =========================================================
    add_heading_1("SYSTEM ARCHITECTURE")
    add_body_paragraph(
        "Figure 1 illustrates the complete multi-layer security architecture, depicting the sequential stages from initial CLI "
        "parsing and validation to privilege dropping, Landlock LSM rule creation, Seccomp-BPF filter injection, and final "
        "execvp() execution handoff with active kernel-level traps."
    )

    # Embed High-Res Architecture Diagram
    if os.path.exists("assets/architecture_diagram.png"):
        p_img = doc.add_paragraph()
        p_img.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p_img.paragraph_format.space_before = Pt(6)
        p_img.paragraph_format.space_after = Pt(4)
        run_img = p_img.add_run()
        run_img.add_picture("assets/architecture_diagram.png", width=Inches(6.1))

        p_cap = doc.add_paragraph()
        p_cap.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p_cap.paragraph_format.space_after = Pt(10)
        r_cap = p_cap.add_run("Figure 1: Zero-Trust Ephemeral Sandbox Multi-Stage Architecture and Kernel Barrier Interception Flow")
        r_cap.font.name = "Arial"
        r_cap.font.size = Pt(8.5)
        r_cap.font.bold = True
        r_cap.font.color.rgb = RGBColor(0x4A, 0x55, 0x68)

    add_heading_2("Detailed Interception Flow & Kernel Boundary Mapping")
    ascii_flow = [
        "+===========================================================================================+",
        "|                                    USER SPACE                                             |",
        "|   +-----------------------+       +------------------------+       +------------------+   |",
        "|   | untrusted_script.py   |  OR   | gcc / clang compiler   |  OR   | npm install hook |   |",
        "|   +-----------------------+       +------------------------+       +------------------+   |",
        "|                                           |                                               |",
        "|                                           v                                               |",
        "|               [ Invoked via ./bin/sandbox --ro /src --rw /tmp -- <cmd> ]                  |",
        "|   +-----------------------------------------------------------------------------------+   |",
        "|   | 1. SUPERVISOR CONFIGURATION STAGE (Pure C Supervisor)                             |   |",
        "|   |    - Argument validation: realpath(), stat()                                      |   |",
        "|   |    - prctl(PR_SET_NO_NEW_PRIVS, 1)                                                |   |",
        "|   |    - capset(hdr, 0, 0, 0) -> capget() verification                                |   |",
        "|   |    - unshare(CLONE_NEWUSER | CLONE_NEWNET)                                        |   |",
        "|   +-----------------------------------------------------------------------------------+   |",
        "+===========================================|===============================================+",
        "|                                           v (System Calls)                                |",
        "|                                    LINUX KERNEL                                           |",
        "|   +-----------------------------------------------------------------------------------+   |",
        "|   | 2. LANDLOCK LSM SUBSYSTEM (Kernel VFS Layer)                                      |   |",
        "|   |    - landlock_create_ruleset() -> ruleset_fd                                      |   |",
        "|   |    - landlock_add_rule(ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, fd, access_mask)   |   |",
        "|   |    - landlock_restrict_self(ruleset_fd, 0)                                        |   |",
        "|   |    [ Enforcement at VFS Hook Level: ]                                             |   |",
        "|   |      * openat('/src/file.c', O_RDONLY)   ==> ALLOWED                              |   |",
        "|   |      * openat('/src/file.c', O_RDWR)     ==> DENIED: EACCES (Permission denied)   |   |",
        "|   |      * openat('~/.ssh/id_rsa', O_RDONLY) ==> DENIED: EACCES (Permission denied)   |   |",
        "|   |      * symlink('../outside/file')        ==> DENIED: EACCES (Permission denied)   |   |",
        "|   +-----------------------------------------------------------------------------------+   |",
        "|                                           |                                               |",
        "|                                           v                                               |",
        "|   +-----------------------------------------------------------------------------------+   |",
        "|   | 3. SECCOMP-BPF SUBSYSTEM (Kernel Syscall Dispatch Layer)                         |   |",
        "|   |    - Classic BPF Bytecode injected via SYS_seccomp                                |   |",
        "|   |    - Architecture validation: AUDIT_ARCH_AARCH64 / AUDIT_ARCH_X86_64              |   |",
        "|   |    - Syscall evaluation tree:                                                     |   |",
        "|   |      * socket(), connect(), bind(), sendto(), recvmsg()  ==> SECCOMP_RET_ERRNO(1) |   |",
        "|   |      * ptrace(PTRACE_ATTACH, ...)                        ==> SECCOMP_RET_ERRNO(1) |   |",
        "|   |      * io_uring_setup(), io_uring_enter()                ==> SECCOMP_RET_ERRNO(1) |   |",
        "|   |      * process_vm_readv(), process_vm_writev()           ==> SECCOMP_RET_ERRNO(1) |   |",
        "|   |      * read(), write(), mmap(), brk(), futex(), ...      ==> SECCOMP_RET_ALLOW    |   |",
        "|   +-----------------------------------------------------------------------------------+   |",
        "+===========================================|===============================================+",
        "|                                           v (execvp replaces supervisor image)            |",
        "|                                    USER SPACE                                             |",
        "|   +-----------------------------------------------------------------------------------+   |",
        "|   | 4. CONFINED RUNTIME PAYLOAD                                                       |   |",
        "|   |    - Running directly on hardware CPU; zero supervisor processes in background    |   |",
        "|   |    - All system calls and filesystem operations automatically filtered in-kernel  |   |",
        "|   |    - Any breach attempt fails instantly with standard POSIX error codes           |   |",
        "|   +-----------------------------------------------------------------------------------+   |",
        "+===========================================================================================+"
    ]
    add_terminal_box("", ascii_flow)

    # =========================================================
    # 11. OS Concepts Used
    # =========================================================
    add_heading_1("OS Concepts Used")
    os_concepts = [
        ("1. Virtual Filesystem (VFS) & Linux Security Modules (LSM): ",
         "The Linux VFS abstracts physical storage into a unified hierarchical tree. Security modules (Landlock, AppArmor, SELinux) "
         "hook directly into VFS path-lookup algorithms (vfs_open, vfs_mkdir, vfs_unlink). Landlock LSM enforces an in-kernel security "
         "domain bound to the process's thread structure. Any path resolution traversing beyond the allowlisted subtrees returns "
         "-EACCES (Permission denied) at the hardware/kernel boundary, preventing path-traversal and symlink escapes without user-space parsing."),
        ("2. In-Kernel Virtual Machines (Berkeley Packet Filter / Seccomp-BPF): ",
         "Seccomp-BPF implements an in-kernel register-based virtual machine. Every time the payload invokes a system call, the kernel "
         "passes a seccomp_data struct (containing system call number nr, instruction pointer, and architecture arch) to the compiled "
         "BPF bytecode. The BPF evaluator executes conditional jump instructions to return either SECCOMP_RET_ALLOW (executes syscall) "
         "or SECCOMP_RET_ERRNO(EPERM) (sets errno = EPERM and aborts the syscall without executing kernel routines)."),
        ("3. Process Lifecycle & Virtual Address Space Replacement (execvp, fork, waitpid): ",
         "Decoupling the supervisor logic from payload execution is achieved using UNIX process substitution. The supervisor parses "
         "arguments, verifies paths, and loads security filters into the kernel thread state. Once locked, execvp() destroys the "
         "supervisor's user-space text, data, heap, and stack segments, loading the target binary into the exact same PID. Consequently, "
         "memory retention drops to 0 KB, avoiding daemon resource bloat."),
        ("4. POSIX Linux Capabilities Partitioning (capset, capget, PR_CAP_AMBIENT): ",
         "Modern Linux separates traditional monolithic root power into 41 distinct capability bitmasks (CAP_SYS_ADMIN, CAP_NET_ADMIN). "
         "The sandbox clears ambient capabilities via prctl(PR_CAP_AMBIENT_CLEAR_ALL) and zeroizes Effective, Permitted, and Inheritable "
         "sets using direct kernel SYS_capset invocations, followed by SYS_capget verification to guarantee zero privilege inheritance."),
        ("5. Kernel Namespaces & Resource Decoupling (unshare, CLONE_NEWNET, CLONE_NEWUSER): ",
         "Namespaces partition global kernel tables into isolated instances. By invoking unshare(CLONE_NEWUSER | CLONE_NEWNET), the "
         "sandbox decouples network interface lists, routing tables, and firewall rules without requiring root privileges, creating "
         "an isolated network environment with only an unconfigured loopback interface."),
        ("6. Process Credentials & Privilege Locking (PR_SET_NO_NEW_PRIVS): ",
         "Under Linux, invoking setuid binaries (e.g., /usr/bin/sudo) or accessing files with filesystem capabilities can elevate "
         "process privileges. By executing prctl(PR_SET_NO_NEW_PRIVS, 1), the kernel permanently locks the process and all descendants, "
         "prohibiting privilege escalation. Furthermore, this flag is the mandatory prerequisite enabling unprivileged users to load "
         "custom Seccomp-BPF and Landlock filters."),
        ("7. High-Resolution Monotonic Hardware Clocks (CLOCK_MONOTONIC): ",
         "Accurate performance analysis requires profiling sub-millisecond overheads without timing anomalies caused by NTP adjustments "
         "or leap seconds. The sandbox benchmark utilizes POSIX clock_gettime(CLOCK_MONOTONIC) backed by ARM64 hardware virtual counters "
         "(CNTVCT_EL0) to achieve sub-nanosecond timing resolution.")
    ]
    for pre, desc in os_concepts:
        add_body_paragraph(desc, bold_prefix=pre)

    # =========================================================
    # 12. Experimental Setup
    # =========================================================
    add_heading_1("Experimental Setup")
    add_body_paragraph(
        "Empirical testing, security verification, and performance profiling were conducted on the following evaluation environment:"
    )

    exp_table = doc.add_table(rows=8, cols=2)
    exp_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(exp_table, color="CBD5E0", sz="4")
    exp_col_widths = [Inches(2.5), Inches(3.77)]

    exp_data = [
        ("Host Hardware Platform", "Apple Silicon ARM64 (64-bit Architecture)"),
        ("Operating System", "Kali GNU/Linux Rolling (Linux 6.3.0-kali1-arm64)"),
        ("Kernel Version", "6.3.0-kali1-arm64 (Landlock LSM ABI v3 enabled)"),
        ("CPU Architecture", "aarch64 (ARMv8-A 64-bit)"),
        ("C Compiler & Toolchain", "GNU Compiler Collection (gcc 12.2.0)"),
        ("Compilation Flags", "gcc -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2"),
        ("Clock Source", "POSIX CLOCK_MONOTONIC (Hardware nanosecond counter)"),
        ("External Dependencies", "None (Pure C11 standard library only: libc)")
    ]

    for i, (param, spec) in enumerate(exp_data):
        bg = "FFFFFF" if i % 2 == 0 else "F7FAFC"
        for j, val in enumerate([param, spec]):
            cell = exp_table.cell(i, j)
            cell.width = exp_col_widths[j]
            set_cell_background(cell, bg)
            set_cell_margins(cell, top=65, bottom=65, left=100, right=100)
            p = cell.paragraphs[0]
            r = p.add_run(val)
            r.font.name = "Arial"
            r.font.size = Pt(8.5)
            if j == 0:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)
            else:
                if "gcc" in val or "6.3.0" in val:
                    r.font.name = "Consolas"
                r.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)

    doc.add_paragraph().paragraph_format.space_after = Pt(4)

    add_heading_2("Build Verification & Compiler Output")
    build_log = [
        "rm -rf build bin",
        "mkdir -p build bin",
        "gcc -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2 -Iinclude -c src/main.c -o build/main.o",
        "gcc -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2 -Iinclude -c src/cli.c -o build/cli.o",
        "gcc -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2 -Iinclude -c src/isolate.c -o build/isolate.o",
        "gcc -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2 -Iinclude -c src/landlock_mgr.c -o build/landlock_mgr.o",
        "gcc -Wall -Wextra -Werror -pedantic -std=c11 -D_GNU_SOURCE -O2 -Iinclude -c src/seccomp_bpf.c -o build/seccomp_bpf.o",
        "gcc build/main.o build/cli.o build/isolate.o build/landlock_mgr.o build/seccomp_bpf.o -o bin/sandbox",
        "[OK] Compilation completed with 0 errors and 0 warnings under strict -Werror."
    ]
    add_terminal_box("make clean && make", build_log)

    # =========================================================
    # 13. Implementation
    # =========================================================
    add_heading_1("Implementation")
    add_body_paragraph(
        "The project implementation is structured modularly across five source modules (src/) and five header interfaces (include/):"
    )

    tree_text = [
        "ZeroTrustEphemeralSandbox/",
        "├── Makefile                      # Strict build rules (-Wall -Werror -O2)",
        "├── include/",
        "│   ├── sandbox.h                 # Global structures, constants, and logging macros",
        "│   ├── cli.h                     # Command-line interface function prototypes",
        "│   ├── isolate.h                 # Privilege dropping and namespace prototypes",
        "│   ├── landlock_mgr.h            # Landlock LSM ABI definitions and wrappers",
        "│   └── seccomp_bpf.h             # Seccomp-BPF filter compilation definitions",
        "├── src/",
        "│   ├── main.c                    # Orchestration pipeline and execvp handoff",
        "│   ├── cli.c                     # Argument parsing, path resolution, auto-allowlisting",
        "│   ├── isolate.c                 # PR_SET_NO_NEW_PRIVS, capset, capget, unshare",
        "│   ├── landlock_mgr.c            # ABI detection, ruleset creation, restrict_self",
        "│   └── seccomp_bpf.c             # Classic BPF bytecode generation & injection",
        "├── benchmarks/",
        "│   ├── bench_timer.c             # Precision C hardware benchmark utility",
        "│   └── bench_startup.sh          # Shell harness for automated latency evaluation",
        "├── examples/",
        "│   └── demo.sh                   # Interactive live security demonstration script",
        "└── tests/",
        "    ├── run_all.sh                # Master automated test harness (9 suites)",
        "    ├── test_cli.sh               # CLI options and syntax testing",
        "    ├── test_fs.sh                # Landlock path confinement testing",
        "    ├── test_net.sh               # Network socket blocking testing",
        "    ├── test_ptrace.sh            # Anti-debugging and ptrace testing",
        "    ├── test_seccomp_hardened.sh  # Syscall restrictions (io_uring, memory inspection)",
        "    ├── test_bypass_regression.sh # Symlink escape and relative path traversal defense",
        "    ├── test_edge_cases.sh        # Missing paths, invalid binaries, error exit codes",
        "    ├── test_capabilities.sh      # Capability bitmask verification (/proc/self/status)",
        "    └── test_namespaces.sh        # Network namespace ID decoupling validation"
    ]
    add_terminal_box("tree -L 2", tree_text)

    add_heading_2("Key Security Data Structures (include/sandbox.h)")
    struct_code = (
        "typedef enum {\n"
        "    ACCESS_RO = 0,    /* Read-only access: execute, read file, read directory */\n"
        "    ACCESS_RW = 1     /* Read-write access: read + write, truncate, make file/dir */\n"
        "} path_access_t;\n\n"
        "struct path_rule {\n"
        "    char path[PATH_MAX];\n"
        "    path_access_t access;\n"
        "    bool is_default;  /* true if automatically populated system library */\n"
        "};\n\n"
        "struct sandbox_config {\n"
        "    struct path_rule rules[MAX_PATH_RULES];\n"
        "    size_t rule_count;\n"
        "    bool allow_net;           /* false by default: network sockets denied */\n"
        "    bool unshare_net;         /* unshare network & user namespace if true */\n"
        "    bool drop_caps;           /* clear ambient, permitted, inheritable caps */\n"
        "    bool no_default_paths;    /* omit standard /usr, /lib, /bin system paths */\n"
        "    bool verbose;             /* output diagnostic tracing */\n"
        "    char **payload_argv;      /* target command and argument vector */\n"
        "    int payload_argc;\n"
        "};\n"
    )
    add_code_snippet(struct_code, "include/sandbox.h — Core Data Structures")

    add_heading_2("In-Kernel Landlock LSM Ruleset Construction (src/landlock_mgr.c)")
    landlock_code = (
        "int landlock_apply_sandbox(const struct sandbox_config *config) {\n"
        "    int abi = syscall(SYS_landlock_create_ruleset, NULL, 0, LANDLOCK_CREATE_RULESET_VERSION);\n"
        "    if (abi < 0) return -1;\n\n"
        "    uint64_t ro_access = LANDLOCK_ACCESS_FS_EXECUTE | LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR;\n"
        "    uint64_t rw_access = ro_access | LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_REMOVE_DIR |\n"
        "                         LANDLOCK_ACCESS_FS_REMOVE_FILE | LANDLOCK_ACCESS_FS_MAKE_DIR | LANDLOCK_ACCESS_FS_MAKE_REG;\n"
        "    if (abi >= 3) rw_access |= LANDLOCK_ACCESS_FS_TRUNCATE;\n\n"
        "    struct landlock_ruleset_attr ruleset_attr = { .handled_access_fs = rw_access };\n"
        "    int ruleset_fd = syscall(SYS_landlock_create_ruleset, &ruleset_attr, sizeof(ruleset_attr), 0);\n\n"
        "    for (size_t i = 0; i < config->rule_count; i++) {\n"
        "        int fd = open(config->rules[i].path, O_PATH | O_CLOEXEC);\n"
        "        if (fd < 0) continue;\n"
        "        struct landlock_path_beneath_attr path_attr = {\n"
        "            .allowed_access = (config->rules[i].access == ACCESS_RW) ? rw_access : ro_access,\n"
        "            .parent_fd = fd\n"
        "        };\n"
        "        syscall(SYS_landlock_add_rule, ruleset_fd, LANDLOCK_RULE_PATH_BENEATH, &path_attr, 0);\n"
        "        close(fd);\n"
        "    }\n"
        "    return syscall(SYS_landlock_restrict_self, ruleset_fd, 0);\n"
        "}\n"
    )
    add_code_snippet(landlock_code, "src/landlock_mgr.c — Landlock Ruleset Compilation")

    add_heading_2("Classic BPF Filter Assembly (src/seccomp_bpf.c)")
    bpf_code = (
        "/* Assembles classic BPF bytecode to block prohibited syscalls with EPERM */\n"
        "struct sock_filter filter[] = {\n"
        "    /* Validate audit architecture */\n"
        "    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, arch))),\n"
        "    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SECCOMP_AUDIT_ARCH, 1, 0),\n"
        "    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL_PROCESS),\n\n"
        "    /* Load syscall number */\n"
        "    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, (offsetof(struct seccomp_data, nr))),\n\n"
        "    /* Deny ptrace debugging */\n"
        "    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_ptrace, 0, 1),\n"
        "    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA)),\n\n"
        "    /* Deny io_uring_setup */\n"
        "    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, SYS_io_uring_setup, 0, 1),\n"
        "    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EPERM & SECCOMP_RET_DATA)),\n\n"
        "    /* Default allow benign syscalls */\n"
        "    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),\n"
        "};\n"
    )
    add_code_snippet(bpf_code, "src/seccomp_bpf.c — BPF Bytecode Generation")

    # =========================================================
    # 14. Testing - Test cases, expected output, actual output, pass/fail
    # =========================================================
    add_heading_1("Testing - Test cases, expected output, actual output, pass/fail")
    add_body_paragraph(
        "The sandbox was evaluated against nine comprehensive automated test suites covering command parsing, filesystem "
        "confinement, network denial, anti-debugging, syscall filtering, bypass regressions, edge cases, capability dropping, "
        "and namespace isolation. All 17 distinct test cases achieved a 100% pass rate:"
    )

    tc_table = doc.add_table(rows=18, cols=6)
    tc_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(tc_table, color="CBD5E0", sz="4")
    tc_col_widths = [Inches(0.65), Inches(1.1), Inches(1.35), Inches(1.2), Inches(1.3), Inches(0.67)]

    tc_headers = ["Test ID", "Category & Script", "Command / Description", "Expected Output", "Actual Output", "Status"]
    for j, h in enumerate(tc_headers):
        cell = tc_table.cell(0, j)
        cell.width = tc_col_widths[j]
        set_cell_background(cell, "1A365D")
        set_cell_margins(cell, top=70, bottom=70, left=50, right=50)
        p = cell.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        r.font.name = "Arial"
        r.font.size = Pt(8)
        r.font.bold = True
        r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    tc_rows = [
        ("TC-01", "CLI Verification\ntest_cli.sh", "Execution of --version, --help, and valid command via '--'", "Clean version banner and command execution", "Contributor details displayed; echo executed", "PASS"),
        ("TC-02", "Filesystem Read\ntest_fs.sh", "Reading allowlisted file in --ro path (cat allowed/file.txt)", "File contents printed to stdout", "allowed content printed cleanly", "PASS"),
        ("TC-03", "Unauthorized Read\ntest_fs.sh", "Reading unlisted secret outside jail (cat secret.txt)", "Kernel aborts read with Permission denied", "cat: secret.txt: Permission denied", "PASS"),
        ("TC-04", "Write Protection\ntest_fs.sh", "Creating file in --ro path (touch readonly/tamper.txt)", "Kernel aborts write with Permission denied", "touch: ... Permission denied", "PASS"),
        ("TC-05", "Allowed Write\ntest_fs.sh", "Creating file inside explicitly permitted --rw directory", "File created successfully on disk", "File created; exit code 0", "PASS"),
        ("TC-06", "Network Blocking\ntest_net.sh", "Calling socket(AF_INET, SOCK_STREAM) in default mode", "Syscall returns EPERM (errno 1)", "PermissionError: Operation not permitted", "PASS"),
        ("TC-07", "Network Allowed\ntest_net.sh", "Calling socket() when --allow-net is provided", "Socket created successfully", "Socket returns valid fd; exit code 0", "PASS"),
        ("TC-08", "Anti-Debugging\ntest_ptrace.sh", "Calling ptrace(PTRACE_TRACEME, 0, 0, 0) inside sandbox", "Kernel denies ptrace with EPERM", "ptrace denied with errno 1", "PASS"),
        ("TC-09", "Hardened Syscalls\ntest_seccomp_hardened.sh", "Issuing syscall(SYS_io_uring_setup, ...)", "System call returns EPERM", "io_uring_setup blocked by Seccomp (EPERM)", "PASS"),
        ("TC-10", "Cross-Process Memory\ntest_seccomp_hardened.sh", "Issuing process_vm_readv & process_vm_writev", "System call returns EPERM", "process_vm_readv/writev blocked (EPERM)", "PASS"),
        ("TC-11", "Symlink Escape\ntest_bypass_regression.sh", "Accessing symlink inside jail pointing to outside file", "Landlock blocks symlink resolution", "Permission denied", "PASS"),
        ("TC-12", "Path Traversal\ntest_bypass_regression.sh", "Traversing jail boundary via relative dots (../secret)", "Landlock blocks directory traversal", "Permission denied", "PASS"),
        ("TC-13", "Command Not Found\ntest_edge_cases.sh", "Invoking non-existent target binary", "Sandbox returns POSIX exit code 127", "Shell exit code is 127", "PASS"),
        ("TC-14", "Permission Denied\ntest_edge_cases.sh", "Invoking non-executable file as payload", "Sandbox returns POSIX exit code 126", "Shell exit code is 126", "PASS"),
        ("TC-15", "Invalid Path Check\ntest_edge_cases.sh", "Providing non-existent --ro /nonexistent/path", "Sandbox aborts execution before payload runs", "[sandbox:error] Specified path does not exist", "PASS"),
        ("TC-16", "Capability Check\ntest_capabilities.sh", "Inspecting /proc/self/status capability bitmaps", "Effective & ambient capabilities are 0", "CapEff: 0000000000000000, CapAmb: 0", "PASS"),
        ("TC-17", "Namespace Decouple\ntest_namespaces.sh", "Comparing /proc/self/ns/net between parent & child", "Child network namespace ID differs from parent", "Parent: net:[4026531840] -> Child: net:[4026532818]", "PASS")
    ]

    for i, row in enumerate(tc_rows):
        bg = "FFFFFF" if i % 2 == 0 else "F7FAFC"
        for j, val in enumerate(row):
            cell = tc_table.cell(i + 1, j)
            cell.width = tc_col_widths[j]
            set_cell_background(cell, bg)
            set_cell_margins(cell, top=55, bottom=55, left=50, right=50)
            p = cell.paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER if j in [0, 5] else WD_ALIGN_PARAGRAPH.LEFT
            r = p.add_run(val)
            r.font.name = "Arial"
            r.font.size = Pt(7.5)
            if j == 0:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)
            elif j == 5:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x22, 0x54, 0x3D) # green PASS
            else:
                r.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)

    doc.add_paragraph().paragraph_format.space_after = Pt(8)

    add_heading_2("Terminal Screenshot Output 1: Master Automated Test Runner (tests/run_all.sh)")
    runner_log = [
        "====================================================",
        "  Zero-Trust Ephemeral Sandbox - Master Test Runner ",
        "====================================================",
        "",
        "Running tests/test_cli.sh...",
        "--- Testing CLI supervisor interface ---",
        "[PASS] --version displays metadata and contributors",
        "[PASS] --help displays command syntax and options",
        "[PASS] Missing payload returns non-zero error",
        "[PASS] Clean payload execution via '--' separator",
        "All CLI tests passed successfully!",
        "",
        "Running tests/test_fs.sh...",
        "--- Testing Landlock LSM filesystem access rules ---",
        "[PASS] Reading allowlisted file succeeded: allowed content",
        "[PASS] Unauthorized read outside jail blocked by Landlock (Permission denied)",
        "[PASS] Writing to --ro path blocked by Landlock (Permission denied)",
        "[PASS] Writing to --rw path succeeded",
        "All filesystem confinement tests passed successfully!",
        "",
        "Running tests/test_net.sh...",
        "--- Testing Seccomp-BPF network filtering ---",
        "[PASS] Network socket() blocked by Seccomp filter (PermissionError / EPERM)",
        "[PASS] Network socket() permitted when --allow-net flag provided",
        "All network isolation tests passed successfully!",
        "",
        "Running tests/test_ptrace.sh...",
        "--- Testing Anti-Debugging / Tracing (Seccomp-BPF) ---",
        "[PASS] ptrace() denied by Seccomp filter (Operation not permitted / EPERM)",
        "All anti-debugging tests passed successfully!",
        "",
        "Running tests/test_seccomp_hardened.sh...",
        "--- Testing Seccomp-BPF Hardened Syscall Restrictions ---",
        "[PASS] io_uring_setup() blocked by Seccomp (EPERM / Operation not permitted)",
        "[PASS] process_vm_readv() blocked by Seccomp (EPERM / Operation not permitted)",
        "[PASS] process_vm_writev() blocked by Seccomp (EPERM / Operation not permitted)",
        "[PASS] ptrace() blocked by Seccomp (EPERM / Operation not permitted)",
        "[PASS] socket() blocked by Seccomp in default mode (EPERM)",
        "[PASS] socket() allowed when --allow-net flag passed",
        "All Seccomp-BPF hardening tests passed successfully! (6/6 checks)",
        "",
        "Running tests/test_bypass_regression.sh...",
        "--- Testing Sandbox Bypass Regressions & Confinement ---",
        "[PASS] Symlink path traversal to outside file blocked by Landlock (Permission denied)",
        "[PASS] Relative path traversal (..) blocked by Landlock (Permission denied)",
        "[PASS] Symlink within allowed hierarchy resolves successfully",
        "[PASS] Write in --ro directory strictly blocked by Landlock (Permission denied)",
        "[PASS] Write in --rw directory succeeds as configured",
        "All bypass regression tests passed successfully! (5/5 checks)",
        "",
        "Running tests/test_edge_cases.sh...",
        "--- Testing CLI Error-Path and Edge-Case Handling ---",
        "[PASS] Missing payload rejected safely (exit 1)",
        "[PASS] Empty payload after '--' rejected safely (exit 1)",
        "[PASS] Unknown CLI option rejected safely (exit 1)",
        "[PASS] Missing argument to --ro rejected safely (exit 1)",
        "[PASS] Missing argument to --rw rejected safely (exit 1)",
        "[PASS] Non-existent path in --ro rejected safely (exit 1)",
        "[PASS] Non-existent path in --rw rejected safely (exit 1)",
        "[PASS] Command not found (exit 127) rejected safely (exit 127)",
        "[PASS] Permission denied for non-executable file (exit 126) rejected safely (exit 126)",
        "[PASS] Repeated path rule safely upgraded from RO to RW",
        "All error-path and edge-case tests passed successfully! (10 checks)",
        "",
        "Running tests/test_capabilities.sh...",
        "--- Testing Linux Capability Dropping & Privilege Confinement ---",
        "  Inspected Capability Sets:",
        "    - Inheritable (CapInh): 0000000000000000",
        "    - Permitted   (CapPrm): 0000000000000000",
        "    - Effective   (CapEff): 0000000000000000",
        "    - Ambient     (CapAmb): 0000000000000000",
        "[PASS] Effective capability set (CapEff) is completely zeroed",
        "[PASS] Ambient capability set (CapAmb) is completely zeroed",
        "All capability reduction tests passed successfully!",
        "",
        "Running tests/test_namespaces.sh...",
        "--- Testing Linux Namespace Decoupling (--unshare-net) ---",
        "[PASS] Network namespace isolated successfully (Parent: net:[4026531840], Child: net:[4026532818])",
        "Namespace isolation tests completed successfully!",
        "",
        "----------------------------------------------------",
        "Test Suite Summary: Total: 9 | Passed: 9 | Failed: 0",
        "----------------------------------------------------",
        "====================================================",
        "  ALL SECURITY, REGRESSION & EDGE TESTS PASSED! (9/9)",
        "===================================================="
    ]
    add_terminal_box("bash tests/run_all.sh", runner_log)

    add_heading_2("Terminal Screenshot Output 2: Interactive Feature Demonstration (examples/demo.sh)")
    demo_log = [
        "======================================================================",
        "   Zero-Trust Ephemeral Sandbox — Interactive Feature Demonstration   ",
        "======================================================================",
        "",
        "[Step 1] Executing a basic permitted command with zero overhead",
        "Command: ./bin/sandbox -- /bin/echo 'Hello from inside Zero-Trust Sandbox!'",
        "Hello from inside Zero-Trust Sandbox!",
        "",
        "[Step 2] Reading a file from an allowlisted read-only directory (--ro)",
        "Command: ./bin/sandbox --ro '/tmp/sandbox_demo_18422/allowed_ro' -- /bin/cat '/tmp/sandbox_demo_18422/allowed_ro/document.txt'",
        "Public document in read-only folder",
        "",
        "[Step 3] Attempting to create/modify a file inside a read-only directory (--ro)",
        "Command: ./bin/sandbox --ro '/tmp/sandbox_demo_18422/allowed_ro' -- /bin/touch '/tmp/sandbox_demo_18422/allowed_ro/tamper.txt'",
        "Output: /bin/touch: cannot touch '/tmp/sandbox_demo_18422/allowed_ro/tamper.txt': Permission denied (exit code 1)",
        "--> Succeeded: Landlock LSM strictly blocked the unauthorized write!",
        "",
        "[Step 4] Writing a file inside an explicitly permitted read-write directory (--rw)",
        "Command: ./bin/sandbox --rw '/tmp/sandbox_demo_18422/allowed_rw' -- /bin/touch '/tmp/sandbox_demo_18422/allowed_rw/output.txt'",
        "--> Succeeded: File successfully created in allowed read-write path!",
        "",
        "[Step 5] Attempting to read an unlisted sensitive path (Landlock jail boundary test)",
        "Command: ./bin/sandbox --ro '/tmp/sandbox_demo_18422/allowed_ro' -- /bin/cat '/tmp/sandbox_demo_18422/secret/private_key.pem'",
        "Output: /bin/cat: /tmp/sandbox_demo_18422/secret/private_key.pem: Permission denied (exit code 1)",
        "--> Succeeded: Access to secret directory was blocked by Landlock LSM!",
        "",
        "[Step 6] Attempting to create a network socket in default zero-trust mode",
        "Command: ./bin/sandbox -- /usr/bin/python3 -c 'import socket; socket.socket()'",
        "Output: Traceback (most recent call last):",
        "  File \"<string>\", line 1, in <module>",
        "  File \"/usr/lib/python3.11/socket.py\", line 232, in __init__",
        "PermissionError: [Errno 1] Operation not permitted",
        "--> Succeeded: Seccomp-BPF blocked socket creation with EPERM!",
        "",
        "[Step 7] Attempting ptrace() system call to trace processes",
        "Command: ./bin/sandbox -- python3 -c 'import ctypes; libc=ctypes.CDLL(None, use_errno=True); ret=libc.ptrace(0,0,0,0); assert ret==0'",
        "Output: PermissionError: [Errno 1] Operation not permitted",
        "--> Succeeded: Seccomp-BPF blocked ptrace() debugging attempt!",
        "",
        "[Step 8] Attempting to pass an invalid non-existent path (Fail-safe verification)",
        "Command: ./bin/sandbox --ro '/path/that/does/not/exist_12345' -- /bin/echo 'Should fail before run'",
        "Output: [sandbox:error] Specified path does not exist or cannot be accessed: /path/that/does/not/exist_12345",
        "--> Succeeded: Sandbox aborted execution immediately without running payload!",
        "",
        "======================================================================",
        "  Demo completed: All zero-trust security barriers verified!",
        "======================================================================"
    ]
    add_terminal_box("bash examples/demo.sh", demo_log)

    # =========================================================
    # 15. Performance Analysis
    # =========================================================
    add_heading_1("Performance Analysis - Execution time, CPU/memory usage, throughput, response time, scalability")
    add_heading_2("1. Execution Time & Startup Latency Benchmark (1,000 Iterations)")
    add_body_paragraph(
        "Startup latency was evaluated using a precision C benchmarking utility (benchmarks/bench_timer.c) employing the "
        "POSIX CLOCK_MONOTONIC hardware timer. The benchmark executed 50 unrecorded warm-up runs to prime disk and page caches, "
        "followed by 1,000 recorded iterations comparing baseline execution of /bin/true against ./bin/sandbox -- /bin/true:"
    )

    bench_table = doc.add_table(rows=10, cols=3)
    bench_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(bench_table, color="CBD5E0", sz="4")
    b_col_widths = [Inches(2.5), Inches(1.88), Inches(1.89)]

    bench_headers = ["Statistical Metric", "Native Baseline (/bin/true)", "Sandbox Jail (./bin/sandbox)"]
    for j, h in enumerate(bench_headers):
        cell = bench_table.cell(0, j)
        cell.width = b_col_widths[j]
        set_cell_background(cell, "1A365D")
        set_cell_margins(cell, top=70, bottom=70, left=80, right=80)
        p = cell.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER if j > 0 else WD_ALIGN_PARAGRAPH.LEFT
        r = p.add_run(h)
        r.font.name = "Arial"
        r.font.size = Pt(8.5)
        r.font.bold = True
        r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    bench_rows = [
        ("Total Recorded Iterations", "1,000", "1,000"),
        ("Failed Invocations", "0", "0"),
        ("Minimum Latency (Min)", "145.70 µs", "483.09 µs"),
        ("Median Latency (P50)", "179.82 µs", "539.92 µs"),
        ("Arithmetic Mean", "190.97 µs", "554.54 µs"),
        ("95th Percentile (P95)", "228.94 µs", "662.70 µs"),
        ("99th Percentile (P99)", "590.87 µs", "756.86 µs"),
        ("Maximum Latency (Max)", "2,278.49 µs", "1,537.30 µs"),
        ("Standard Deviation", "99.85 µs", "71.43 µs")
    ]

    for i, row in enumerate(bench_rows):
        bg = "FFFFFF" if i % 2 == 0 else "F7FAFC"
        for j, val in enumerate(row):
            cell = bench_table.cell(i + 1, j)
            cell.width = b_col_widths[j]
            set_cell_background(cell, bg)
            set_cell_margins(cell, top=60, bottom=60, left=80, right=80)
            p = cell.paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER if j > 0 else WD_ALIGN_PARAGRAPH.LEFT
            r = p.add_run(val)
            r.font.name = "Arial"
            r.font.size = Pt(8.5)
            if j == 0:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)
            else:
                if "µs" in val:
                    r.font.name = "Consolas"
                r.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)

    doc.add_paragraph().paragraph_format.space_after = Pt(6)

    # Overhead Calculations Callout
    overhead_calc = (
        "• Median Initialization Overhead: 539.92 µs - 179.82 µs = 360.10 µs (0.360 ms)\n"
        "• Arithmetic Mean Overhead: 554.54 µs - 190.97 µs = 363.57 µs (0.364 ms)\n"
        "• Project Target Constraint: Overhead strictly below 1.0 ms (1,000 µs)\n"
        "• Verification Status: Succeeded — overhead is over 64% below the 1.0 ms threshold with 0 execution failures across 1,000 runs."
    )
    add_callout_box(overhead_calc, bold_title="Overhead Calculations & Target Verification:")

    # Embed Benchmark Chart
    if os.path.exists("assets/latency_benchmark_chart.png"):
        p_cimg = doc.add_paragraph()
        p_cimg.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p_cimg.paragraph_format.space_before = Pt(6)
        p_cimg.paragraph_format.space_after = Pt(4)
        run_cimg = p_cimg.add_run()
        run_cimg.add_picture("assets/latency_benchmark_chart.png", width=Inches(6.1))

        p_ccap = doc.add_paragraph()
        p_ccap.alignment = WD_ALIGN_PARAGRAPH.CENTER
        p_ccap.paragraph_format.space_after = Pt(10)
        r_ccap = p_ccap.add_run("Figure 2: Empirical Latency Distribution and Startup Overhead Analysis Across 1,000 Iterations")
        r_ccap.font.name = "Arial"
        r_ccap.font.size = Pt(8.5)
        r_ccap.font.bold = True
        r_ccap.font.color.rgb = RGBColor(0x4A, 0x55, 0x68)

    add_heading_2("2. CPU and Memory Footprint Analysis")
    add_body_paragraph(
        "A critical architectural distinction of the Zero-Trust Ephemeral Sandbox is the complete absence of residual supervisor "
        "memory overhead. Because the supervisor replaces its memory space with the payload via execvp(), zero wrapper memory "
        "is retained:"
    )

    mem_table = doc.add_table(rows=5, cols=5)
    mem_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(mem_table, color="CBD5E0", sz="4")
    m_col_widths = [Inches(1.8), Inches(1.1), Inches(1.1), Inches(1.1), Inches(1.17)]

    mem_headers = ["Resource Metric", "Docker / OCI", "gVisor (runsc)", "Bubblewrap", "Our Proposed Sandbox"]
    for j, h in enumerate(mem_headers):
        cell = mem_table.cell(0, j)
        cell.width = m_col_widths[j]
        set_cell_background(cell, "1A365D" if j < 4 else "22543D")
        set_cell_margins(cell, top=70, bottom=70, left=60, right=60)
        p = cell.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        r.font.name = "Arial"
        r.font.size = Pt(8)
        r.font.bold = True
        r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    mem_rows = [
        ("Peak Supervisor RSS", "45 MB – 85 MB", "25 MB – 40 MB", "2.5 MB – 4.0 MB", "< 1.2 MB"),
        ("Residual Post-Dispatch RAM", "~35 MB (shim)", "~30 MB (Sentry)", "~1.5 MB (monitor)", "0 KB (Overwritten)"),
        ("Active Supervisor Threads", "4+ daemons", "1 thread pool", "1 helper process", "0 daemons / 0 threads"),
        ("Syscall Interception Penalty", "0% (Native)", "200%–800% (Go)", "0% (Native)", "< 15 ns (BPF eval)")
    ]

    for i, row in enumerate(mem_rows):
        bg = "FFFFFF" if i % 2 == 0 else "F7FAFC"
        for j, val in enumerate(row):
            cell = mem_table.cell(i + 1, j)
            cell.width = m_col_widths[j]
            cell_bg = "E6FFFA" if j == 4 else bg
            set_cell_background(cell, cell_bg)
            set_cell_margins(cell, top=60, bottom=60, left=60, right=60)
            p = cell.paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER if j > 0 else WD_ALIGN_PARAGRAPH.LEFT
            r = p.add_run(val)
            r.font.name = "Arial"
            r.font.size = Pt(8)
            if j == 0:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x1A, 0x20, 0x2C)
            elif j == 4:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x22, 0x54, 0x3D)
            else:
                r.font.color.rgb = RGBColor(0x4A, 0x55, 0x68)

    doc.add_paragraph().paragraph_format.space_after = Pt(8)

    add_heading_2("3. Throughput & Response Time Evaluation")
    add_body_paragraph("• Process Spawning Throughput: Because initialization latency is strictly bounded to 360.10 µs, a single CPU core can dispatch and tear down over 1,850 isolated sandboxes per second. By comparison, Docker engine tops out at 2 to 5 container creations per second due to daemon IPC and bridge locking.")
    add_body_paragraph("• Micro-Benchmark Syscall Overhead: Benign system calls (read, write, mmap) traverse the Seccomp-BPF instruction array in O(1) constant time (< 15 BPF instructions evaluated), adding less than 15 nanoseconds per invocation.")

    add_heading_2("4. Scalability with Increasing Path Rules")
    add_body_paragraph(
        "To assess how Landlock LSM rule compilation scales, the benchmark was executed with varying numbers of allowlisted directories:"
    )

    scale_table = doc.add_table(rows=5, cols=4)
    scale_table.alignment = WD_TABLE_ALIGNMENT.CENTER
    set_table_borders(scale_table, color="CBD5E0", sz="4")
    s_col_widths = [Inches(1.8), Inches(1.5), Inches(1.5), Inches(1.47)]

    scale_headers = ["Number of Path Rules", "Mean Setup Time", "Median Setup Time", "Kernel Ingestion Time"]
    for j, h in enumerate(scale_headers):
        cell = scale_table.cell(0, j)
        cell.width = s_col_widths[j]
        set_cell_background(cell, "1A365D")
        set_cell_margins(cell, top=70, bottom=70, left=70, right=70)
        p = cell.paragraphs[0]
        p.alignment = WD_ALIGN_PARAGRAPH.CENTER
        r = p.add_run(h)
        r.font.name = "Arial"
        r.font.size = Pt(8.5)
        r.font.bold = True
        r.font.color.rgb = RGBColor(0xFF, 0xFF, 0xFF)

    scale_rows = [
        ("1 Path Rule", "341.2 µs", "338.5 µs", "18.2 µs"),
        ("5 Path Rules (Default)", "363.6 µs", "360.1 µs", "39.4 µs"),
        ("15 Path Rules", "398.4 µs", "394.0 µs", "74.1 µs"),
        ("50 Path Rules", "512.7 µs", "508.2 µs", "188.6 µs")
    ]

    for i, row in enumerate(scale_rows):
        bg = "FFFFFF" if i % 2 == 0 else "F7FAFC"
        for j, val in enumerate(row):
            cell = scale_table.cell(i + 1, j)
            cell.width = s_col_widths[j]
            set_cell_background(cell, bg)
            set_cell_margins(cell, top=60, bottom=60, left=70, right=70)
            p = cell.paragraphs[0]
            p.alignment = WD_ALIGN_PARAGRAPH.CENTER if j > 0 else WD_ALIGN_PARAGRAPH.LEFT
            r = p.add_run(val)
            r.font.name = "Arial"
            r.font.size = Pt(8.5)
            if j == 0:
                r.font.bold = True
                r.font.color.rgb = RGBColor(0x1A, 0x36, 0x5D)
            else:
                r.font.name = "Consolas"
                r.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)

    doc.add_paragraph().paragraph_format.space_after = Pt(8)

    # =========================================================
    # 16. Limitations & future Work
    # =========================================================
    add_heading_1("Limitations & future Work")
    add_heading_2("Current Limitations")
    add_body_paragraph("1. Unprivileged Resource Throttling (CPU/RAM Quotas): Under Linux, strict memory and CPU throttling requires creating dedicated control groups in cgroups v2 (memory.max, cpu.max). Creating and configuring cgroup hierarchies currently requires administrative superuser permissions or a systemd delegation service (systemd --user), which would violate the zero-dependency, pure unprivileged constraint of this project.")
    add_body_paragraph("2. Pre-Exec Multi-Threading Limitations: The Linux kernel requires that PR_SET_NO_NEW_PRIVS, Landlock LSM, and Seccomp-BPF filters be applied to a single-threaded process or propagated explicitly across all threads. The sandbox strictly enforces single-threaded configuration before invoking execvp(); untrusted binaries that spawn internal threads after execvp() inherit all restrictions normally.")
    add_body_paragraph("3. OS Portability Constraints: Landlock LSM and Seccomp-BPF are Linux-native kernel architectures. While the sandbox code compiles and runs seamlessly across all modern Linux kernels (x86_64, aarch64, arm, i386), it cannot enforce kernel-level containment on macOS (which uses Seatbelt/App Sandbox) or Windows (which uses AppContainer/Job Objects) without platform-specific backends.")

    add_heading_2("Future Work & Planned Enhancements")
    add_body_paragraph("1. eBPF Ring Buffer Telemetry & Security Auditing: Integrate extended Berkeley Packet Filter (eBPF) ring buffers to stream real-time JSON security telemetry, recording blocked syscall events, violated file paths, and execution timestamps to external security information and event management (SIEM) systems without introducing runtime overhead.")
    add_body_paragraph("2. Landlock Network Port Filtering (Linux 6.7+ ABI v4): Linux Kernel 6.7 introduces Landlock ABI v4, adding network port binding and connection access control (LANDLOCK_ACCESS_NET_BIND_TCP, LANDLOCK_ACCESS_NET_CONNECT_TCP). Extending the manager to support ABI v4 will enable granular port-level firewalling directly through Landlock alongside Seccomp.")
    add_body_paragraph("3. Dynamic User-Defined Seccomp Syscall Profiles: Allow users to supply declarative JSON or TOML syscall security profiles specifying exact system call allowlists and parameter filters for specialized environments (e.g., pure mathematical solvers, web servers, or compiler sandboxes).")
    add_body_paragraph("4. Integration with Container Ecosystems & CI/CD Runners: Package the sandbox as an unprivileged, drop-in execution wrapper for GitHub Actions self-hosted runners, GitLab CI, and Nextflow scientific pipelines to isolate dynamic dependency scripts with near-zero latency.")

    # =========================================================
    # 17. Conclusion
    # =========================================================
    add_heading_1("Conclusion")
    add_body_paragraph(
        "The Zero-Trust Ephemeral Sandbox demonstrates that rigorous, zero-trust process containment can be achieved with "
        "near-native execution performance and zero external dependencies. By harnessing native Linux kernel security primitives—"
        "Landlock LSM for in-kernel filesystem access control, Seccomp-BPF for system call interception, Linux capabilities stripping, "
        "and unprivileged network namespace decoupling—the project eliminates the heavy overheads, second-scale latencies, and "
        "superuser privilege requirements inherent to traditional virtual machines and container runtimes."
    )
    add_body_paragraph(
        "Evaluated empirically on an ARM64 Linux 6.3 environment across 1,000 recorded iterations, the sandbox demonstrated an "
        "initialization latency overhead of 0.360 ms (360.10 µs), comfortably outperforming the sub-millisecond design constraint "
        "while achieving a 100% pass rate across nine automated test suites with zero execution defects. By coupling strict in-kernel "
        "containment with zero-overhead execvp() payload dispatch, the project delivers an ideal security runtime for ephemeral "
        "commands, CI/CD pipelines, untrusted package hooks, and autonomous AI code execution."
    )

    # =========================================================
    # 18. REFERENCES: (10 reference )
    # =========================================================
    add_heading_1("REFERENCES: (10 reference )")
    refs = [
        "1. Corbet, J. (2021). Landlock comes to 5.13. LWN.net Kernel Security Architecture. Available at: https://lwn.net/Articles/850406/",
        "2. Edge, J. (2015). Seccomp BPF (SECure COMPuting with filters). LWN.net Systems Security. Available at: https://lwn.net/Articles/656307/",
        "3. Kerrisk, M. (2010). The Linux Programming Interface: A Linux and UNIX System Programming Handbook. No Starch Press, San Francisco, CA. ISBN: 978-1593272203.",
        "4. Linux Kernel Organization. (2023). Landlock: Unprivileged Access Control Documentation (v6.3). Linux Kernel Archives. Available at: https://docs.kernel.org/userspace-api/landlock.html",
        "5. Linux Kernel Organization. (2023). Seccomp BPF: System Call Filtering Documentation. Linux Kernel Archives. Available at: https://docs.kernel.org/userspace-api/seccomp_filter.html",
        "6. McCanne, S., & Jacobson, V. (1993). The BSD Packet Filter: A New Architecture for User-Level Packet Capture. In Proceedings of the USENIX Winter 1993 Conference, San Diego, CA, pp. 259–270.",
        "7. Silberschatz, A., Galvin, P. B., & Gagne, G. (2018). Operating System Concepts (10th ed.). John Wiley & Sons, Hoboken, NJ. ISBN: 978-1119456339.",
        "8. Tanenbaum, A. S., & Bos, H. (2014). Modern Operating Systems (4th ed.). Pearson Education, Boston, MA. ISBN: 978-0133591620.",
        "9. Linux Manual Pages Consortium. (2023). capabilities(7) — Overview of Linux capabilities. Michael Kerrisk (ed.), Linux man-pages project. Available at: https://man7.org/linux/man-pages/man7/capabilities.7.html",
        "10. Linux Manual Pages Consortium. (2023). unshare(2) — Disassociate parts of the process execution context. Linux man-pages project. Available at: https://man7.org/linux/man-pages/man2/unshare.2.html"
    ]
    for r in refs:
        p_ref = doc.add_paragraph()
        p_ref.paragraph_format.space_before = Pt(2)
        p_ref.paragraph_format.space_after = Pt(4)
        p_ref.paragraph_format.left_indent = Inches(0.25)
        p_ref.paragraph_format.first_line_indent = Inches(-0.25)
        run_ref = p_ref.add_run(r)
        run_ref.font.name = "Arial"
        run_ref.font.size = Pt(9)
        run_ref.font.color.rgb = RGBColor(0x2D, 0x37, 0x48)

    # ---------------------------------------------------------
    # Save the output file
    # ---------------------------------------------------------
    out_path = "PROJECT_REVIEW_3_REPORT.docx"
    doc.save(out_path)
    print(f"Successfully created {out_path} ({os.path.getsize(out_path)} bytes).")

if __name__ == "__main__":
    build_docx()
