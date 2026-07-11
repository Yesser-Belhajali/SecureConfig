import pdfplumber

def extractor():
    texte_brut=""
    with pdfplumber.open("data/CIS_Ubuntu_Linux_24.04_LTS_Benchmark_v2.0.0.pdf") as pdf:
        for i in range(25,901):
            page=pdf.pages[i]
            texte=page.extract_text()
            texte_brut+=texte+"\n"
    f=open("output/res.txt","w")
    f.write(texte_brut)
    f.close()
extractor()