import pdfplumber
import re

from PagesListExtractor import extract_pdf_text

liste_num_pages = extract_pdf_text("data/CIS_Ubuntu_Linux_24.04_LTS_Benchmark_v2.0.0.pdf")

def RuleExtractor(liste_num_pages):
    text_brut=""
    with pdfplumber.open("data/CIS_Ubuntu_Linux_24.04_LTS_Benchmark_v2.0.0.pdf") as pdf:
        for i in range(len(liste_num_pages)-1):
            if(liste_num_pages[i] in {61,73,82,89,97,106,115,124,146,150,161,167,211,233,260,316,329,335,343,352,374,378,389,408,484,517,529,589,604,614,626,636,661,669,680,697,716,728,751,772,778,789,799,881,904,914,946,973}):
                continue
            text_Rule=""
            debut = liste_num_pages[i]
            fin = liste_num_pages[i+1]
            for j in range(debut, fin):
                page = pdf.pages[j]
                texte = page.extract_text()
                if texte:
                    lignes=texte.split("\n")
                    lignes_propres=lignes[:-2]
                    texte="\n".join(lignes_propres)
                    text_Rule += texte + "\n"
            header=header_extractor(text_Rule)
            assessment=assessment_status_extractor(header)
            print(header,assessment)
            """id=id_extractor(header)
            titre=title_extractor(header)
            text_brut+=id+"   haha   "+titre+"   haha   "+str(assessment)+"\n"
    f=open("res.txt","w")
    f.write(text_brut)
    f.close()"""
    

def header_extractor(text_Rule):
    lignes=text_Rule.split("Profile Applicability:")
    titre=lignes[0].split("\n")
    if len(titre)==1:
        return titre[0]
    else:
        return titre[0]+" "+titre[1]

def id_extractor(header):
    premier_espace=header.find(" ")
    id=header[:premier_espace]
    if id:
        return id
    return None

def title_extractor(header):
    premier_espace=header.find(" ")
    dernier_espace=header.rfind(" ")
    titre=header[premier_espace+1:dernier_espace]
    if titre:
        return titre
    return None


def assessment_status_extractor(header):
    parenthese_gauche=header.find("(")
    parenthese_droite=header.find(")")
    assessment=header[parenthese_gauche+1:parenthese_droite]
    if assessment:
        return assessment
    return None






RuleExtractor(liste_num_pages)