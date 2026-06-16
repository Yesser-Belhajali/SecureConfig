import pdfplumber
import re
import bisect

def extract_pdf_text(path):
    liste_num_pages=[]
    texte_brut=""
    with pdfplumber.open(path) as pdf:
        for i in range(2,11):
            texte_page=""
            page=pdf.pages[i]
            texte_page=page.extract_text()
            texte_page=nettoyage_page(texte_page)
            if texte_page:
                texte_brut+=texte_page+"\n"
    f=open("output/res.txt","w")
    f.write(texte_brut)
    f.close()
    with open("output/res.txt","r") as res:
        for ligne in res:
            match=re.search(r"(\d+)\s*$",ligne)
            if match:
                liste_num_pages.append(int(match.group(1)))
    liste_ajouter=[61,73,82,89,97,106,115,124,146,150,161,167,211,233,260,316,329,335,343,352,374,378,389,408,484,517,529,589,604,614,626,636,661,669,680,697,716,728,751,772,778,789,799,881,904,914,946,973]
    for elt in liste_ajouter:
        bisect.insort(liste_num_pages,elt)
    liste_num_pages.remove(330)
    return liste_num_pages


def nettoyage_page(texte):
    lignes=texte.split("\n")
    lignes_propres=[]
    for ligne in lignes:
        ligne=ligne.strip()
        if "Ensure" in ligne or "(Manual)" in ligne or "(Automated)" in ligne or ligne.startswith(".............."):
            lignes_propres.append(ligne)
    return "\n".join(lignes_propres)