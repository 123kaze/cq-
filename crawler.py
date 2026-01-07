import requests
from bs4 import BeautifulSoup
import re
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
from datetime import datetime

plt.rcParams["font.sans-serif"] = [
    "PingFang HK",
    "Noto Sans CJK SC",
    "Noto Sans CJK JP",
    "SimHei",
    "DejaVu Sans",
    "sans-serif",
]
plt.rcParams["axes.unicode_minus"] = False


def get_html(url):
    h = {
        "User-Agent": "Mozilla/5.0",
        "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
    }
    r = requests.get(url, headers=h, timeout=10)
    r.encoding = "utf-8" if r.encoding == "ISO-8859-1" else r.apparent_encoding
    return r.text


def extract_data(html):
    soup = BeautifulSoup(html, "html.parser")
    text = soup.get_text()
    text = re.sub(r"\s+", " ", text)

    data = []
    p_pattern = r"(\d+)[、.]?\s*([\u4e00-\u9fa5]+(?:省|市|区|自治区))"
    t_pattern = r"总发电量([\d\.]+)亿千瓦时"
    y_pattern = r"同比增长([-\d\.]+)%"

    p_matches = list(re.finditer(p_pattern, text))

    for i, m in enumerate(p_matches):
        p_name = m.group(2)

        start = m.end()
        if i < len(p_matches) - 1:
            end = p_matches[i + 1].start()
        else:
            end = len(text)

        p_text = text[start:end]

        m_matches = list(re.finditer(r"2024年([1-9]|1[0-2])月", p_text))
        q_matches = list(re.finditer(r"1-([1-9])月", p_text))

        for mm in m_matches:
            month = mm.group(1)
            m_start = max(0, mm.start() - 200)
            m_end = min(len(p_text), mm.end() + 500)
            m_text = p_text[m_start:m_end]

            t_match = re.search(t_pattern, m_text)
            if t_match:
                d = {
                    "省份": p_name,
                    "月份": month,
                    "期间": "月度",
                    "总发电量": float(t_match.group(1)),
                }

                y_match = re.search(y_pattern, m_text)
                if y_match:
                    d["总发电量_同比"] = float(y_match.group(1))

                extract_types(d, m_text)
                data.append(d)

        for qm in q_matches:
            month = qm.group(1)
            q_start = max(0, qm.start() - 200)
            q_end = min(len(p_text), qm.end() + 500)
            q_text = p_text[q_start:q_end]

            t_match = re.search(t_pattern, q_text)
            if t_match:
                d = {
                    "省份": p_name,
                    "月份": f"1-{month}",
                    "期间": "季度累计",
                    "总发电量": float(t_match.group(1)),
                }

                y_match = re.search(y_pattern, q_text)
                if y_match:
                    d["总发电量_同比"] = float(y_match.group(1))

                extract_types(d, q_text)
                data.append(d)

    return data


def get_q1_data(data_list):
    q1 = []
    for item in data_list:
        if item["期间"] == "月度":
            m = int(item["月份"])
            if 1 <= m <= 3:
                q1.append(item)
        elif item["期间"] == "季度累计":
            if item["月份"] in ["1-3", "3"]:
                q1.append(item)
    return q1


def summary_data(data_list):
    df = pd.DataFrame(data_list)

    if len(df) == 0:
        return {}

    s = {
        "记录数": len(df),
        "省份数": df["省份"].nunique(),
        "月份范围": f"{df['月份'].min()} - {df['月份'].max()}",
        "总发电量": df["总发电量"].sum() if "总发电量" in df.columns else 0,
    }

    if "省份" in df.columns and "总发电量" in df.columns:
        p_sum = df.groupby("省份")["总发电量"].sum().sort_values(ascending=False)
        s["各省排名"] = p_sum.head(10).to_dict()

    types = ["火电", "水电", "风电", "光伏", "核电"]
    for t in types:
        if t in df.columns:
            s[f"{t}总量"] = df[t].sum()

    return s


def extract_types(d, text):
    type_map = {
        "火电": ["火电", "火力"],
        "水电": ["水电", "水力"],
        "风电": ["风电", "风力"],
        "光伏": ["光伏", "太阳能"],
        "核电": ["核电"],
    }

    for main, vars in type_map.items():
        for var in vars:
            patterns = [
                f"{var}发电量([\\d\\.]+)亿千瓦时",
                f"{var}(?:发电量)?([\\d\\.]+)亿千瓦时",
                f"{var}[：:]?([\\d\\.]+)亿千瓦时",
            ]

            for pat in patterns:
                m = re.search(pat, text)
                if m:
                    try:
                        d[main] = float(m.group(1))

                        y_pats = [
                            f"{var}(?:发电量)?[\\d\\.]+亿千瓦时[^%\\d]*([-\\d\\.]+)%",
                            f"{var}[^%\\d]*([-\\d\\.]+)%",
                        ]

                        for y_pat in y_pats:
                            y_m = re.search(y_pat, text)
                            if y_m:
                                try:
                                    d[f"{main}_同比"] = float(y_m.group(1))
                                except:
                                    pass
                                break

                    except:
                        pass
                    break
            else:
                continue
            break


def save_excel(data_list, filename="power_data.xlsx"):
    df = pd.DataFrame(data_list)
    df = df.drop_duplicates(subset=["省份", "月份", "期间"], keep="first")

    order = [
        "省份",
        "月份",
        "期间",
        "总发电量",
        "总发电量_同比",
        "火电",
        "火电_同比",
        "水电",
        "水电_同比",
        "风电",
        "风电_同比",
        "光伏",
        "光伏_同比",
        "核电",
        "核电_同比",
    ]

    exist = [col for col in order if col in df.columns]
    other = [col for col in df.columns if col not in exist]
    df = df[exist + other]

    def m_key(x):
        if isinstance(x, str) and "1-" in x:
            return int(x.split("-")[1]) + 12
        return int(x)

    df = df.sort_values(
        ["省份", "月份"], key=lambda x: x.map(m_key) if x.name == "月份" else x
    )

    df.to_excel(filename, index=False)
    print(f"保存到 {filename}")


def plot_data(data_list):
    df = pd.DataFrame(data_list)
    m_df = df[df["期间"] == "月度"]

    fig = plt.figure(figsize=(20, 15))
    fig.suptitle("2024年各省发电量分析", fontsize=20, fontweight="bold")

    ax1 = plt.subplot(2, 3, 1)
    p_totals = (
        m_df.groupby("省份")["总发电量"].sum().sort_values(ascending=False).head(10)
    )
    c1 = plt.cm.viridis(np.linspace(0.3, 0.9, len(p_totals)))
    bars1 = ax1.bar(range(len(p_totals)), p_totals.values, color=c1)
    ax1.set_xlabel("省份")
    ax1.set_ylabel("总发电量 (亿千瓦时)")
    ax1.set_title("各省总发电量排名")
    ax1.set_xticks(range(len(p_totals)))
    ax1.set_xticklabels(p_totals.index, rotation=45, ha="right")

    for i, (bar, v) in enumerate(zip(bars1, p_totals.values)):
        h = bar.get_height()
        ax1.text(
            bar.get_x() + bar.get_width() / 2.0,
            h + h * 0.01,
            f"{v:.1f}",
            ha="center",
            va="bottom",
            fontsize=9,
        )

    ax2 = plt.subplot(2, 3, 2)
    if "水电" in m_df.columns:
        h_totals = (
            m_df.groupby("省份")["水电"].sum().sort_values(ascending=False).head(10)
        )
        c2 = plt.cm.Blues(np.linspace(0.3, 0.9, len(h_totals)))
        bars2 = ax2.bar(range(len(h_totals)), h_totals.values, color=c2)
        ax2.set_xlabel("省份")
        ax2.set_ylabel("水力发电量 (亿千瓦时)")
        ax2.set_title("各省水力发电量排名")
        ax2.set_xticks(range(len(h_totals)))
        ax2.set_xticklabels(h_totals.index, rotation=45, ha="right")

        for i, (bar, v) in enumerate(zip(bars2, h_totals.values)):
            h = bar.get_height()
            ax2.text(
                bar.get_x() + bar.get_width() / 2.0,
                h + h * 0.01,
                f"{v:.1f}",
                ha="center",
                va="bottom",
                fontsize=9,
            )
    else:
        ax2.text(
            0.5,
            0.5,
            "无水力发电数据",
            ha="center",
            va="center",
            transform=ax2.transAxes,
        )

    ax3 = plt.subplot(2, 3, 3)
    if "火电" in m_df.columns:
        t_totals = (
            m_df.groupby("省份")["火电"].sum().sort_values(ascending=False).head(10)
        )
        c3 = plt.cm.Reds(np.linspace(0.3, 0.9, len(t_totals)))
        bars3 = ax3.bar(range(len(t_totals)), t_totals.values, color=c3)
        ax3.set_xlabel("省份")
        ax3.set_ylabel("火力发电量 (亿千瓦时)")
        ax3.set_title("各省火力发电量排名")
        ax3.set_xticks(range(len(t_totals)))
        ax3.set_xticklabels(t_totals.index, rotation=45, ha="right")

        for i, (bar, v) in enumerate(zip(bars3, t_totals.values)):
            h = bar.get_height()
            ax3.text(
                bar.get_x() + bar.get_width() / 2.0,
                h + h * 0.01,
                f"{v:.1f}",
                ha="center",
                va="bottom",
                fontsize=9,
            )
    else:
        ax3.text(
            0.5,
            0.5,
            "无火力发电数据",
            ha="center",
            va="center",
            transform=ax3.transAxes,
        )

    ax4 = plt.subplot(2, 3, 4)
    types = ["火电", "水电", "风电", "光伏", "核电"]
    t_totals = []
    t_labels = []

    for t in types:
        if t in m_df.columns:
            total = m_df[t].sum()
            if total > 0:
                t_totals.append(total)
                t_labels.append(t)

    if len(t_totals) > 0:
        c4 = plt.cm.Set3(np.linspace(0, 1, len(t_totals)))
        ax4.pie(t_totals, labels=t_labels, autopct="%1.1f%%", colors=c4, startangle=90)
        ax4.set_title("全国发电类型构成")
        ax4.axis("equal")
    else:
        ax4.text(
            0.5,
            0.5,
            "无发电类型数据",
            ha="center",
            va="center",
            transform=ax4.transAxes,
        )

    ax5 = plt.subplot(2, 3, 5)
    if "水电" in m_df.columns and "总发电量" in m_df.columns:
        h_ratio = m_df["水电"].sum() / m_df["总发电量"].sum() * 100
        o_ratio = 100 - h_ratio
        ax5.pie(
            [h_ratio, o_ratio],
            labels=["水力发电", "其他"],
            autopct="%1.1f%%",
            colors=["#66b3ff", "#ffcc99"],
            startangle=90,
        )
        ax5.set_title("水力发电占比")
        ax5.axis("equal")
    else:
        ax5.text(
            0.5,
            0.5,
            "无水力发电数据",
            ha="center",
            va="center",
            transform=ax5.transAxes,
        )

    ax6 = plt.subplot(2, 3, 6)
    if "火电" in m_df.columns and "总发电量" in m_df.columns:
        t_ratio = m_df["火电"].sum() / m_df["总发电量"].sum() * 100
        o_ratio = 100 - t_ratio
        ax6.pie(
            [t_ratio, o_ratio],
            labels=["火力发电", "其他"],
            autopct="%1.1f%%",
            colors=["#ff9999", "#99ff99"],
            startangle=90,
        )
        ax6.set_title("火力发电占比")
        ax6.axis("equal")
    else:
        ax6.text(
            0.5,
            0.5,
            "无火力发电数据",
            ha="center",
            va="center",
            transform=ax6.transAxes,
        )

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    out = f"发电量分析_{ts}.png"
    plt.savefig(out, dpi=300, bbox_inches="tight")
    print(f"图表保存为 '{out}'")
    plt.show()


def main():
    url = "https://www.hxny.com/nd-102461-0-17.html"
    print(f"从 {url} 获取数据...")

    html = get_html(url)
    data = extract_data(html)

    print(f"提取到 {len(data)} 条数据")

    if data:
        q1 = get_q1_data(data)
        print(f"2024年1-3月数据: {len(q1)} 条")

        s = summary_data(q1)
        print("\n数据汇总:")
        for k, v in s.items():
            if k == "各省排名":
                print(f"{k}:")
                for p, pw in v.items():
                    print(f"  {p}: {pw:.1f} 亿千瓦时")
            else:
                print(f"{k}: {v}")

        print("\n前5条数据示例:")
        for i, item in enumerate(q1[:5]):
            print(f"{i+1}. {item}")

        ts = datetime.now().strftime("%Y%m%d_%H%M%S")
        fname = f"2024年1-3月发电量_{ts}.xlsx"
        save_excel(q1, fname)
        plot_data(q1)


if __name__ == "__main__":
    main()
