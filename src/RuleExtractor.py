import pdfplumber
import json

from PagesListExtractor import extract_pdf_text



liste_num_pages = extract_pdf_text("data/CIS_Ubuntu_Linux_24.04_LTS_Benchmark_v2.0.0.pdf")

def RuleExtractor(liste_num_pages):
    rules=[]
    with pdfplumber.open("data/CIS_Ubuntu_Linux_24.04_LTS_Benchmark_v2.0.0.pdf") as pdf:
        for i in range(len(liste_num_pages)-1):
            if(liste_num_pages[i] in {61,73,82,89,97,106,115,124,146,150,161,167,211,233,260,316,329,335,343,352,374,378,389,408,484,517,529,589,604,614,626,636,661,669,680,697,716,728,751,772,778,789,799,881,904,914,946,973}):
                continue
            texte_Rule=""
            debut = liste_num_pages[i]
            fin = liste_num_pages[i+1]
            for j in range(debut, fin):
                page = pdf.pages[j]
                texte = page.extract_text()
                if texte:
                    lignes=texte.split("\n")
                    lignes_propres=lignes[:-2]
                    texte="\n".join(lignes_propres)
                    texte_Rule += texte + "\n"
            header=header_extractor(texte_Rule)
            rule={
                "id":id_extractor(header),
                "title":title_extractor(header),
                "assessment_status":assessment_status_extractor(header),
                "profile":profile_extractor(texte_Rule),
                "description":description_extractor(texte_Rule),
                "rationale":rationale_extractor(texte_Rule),
                "impact":impact_extractor(texte_Rule),
                "audit":audit_extractor(texte_Rule),
                "remediation":remediation_extractor(texte_Rule),
                "default_value":default_value_extractor(texte_Rule),
                "references":references_extractor(texte_Rule),
                "additional_information":additional_info_extractor(texte_Rule),
                "cis_controls":cis_controls_extractor(texte_Rule)
            }
            rules.append(rule)
    with open("output/rules.json","w",encoding="utf-8") as f:
        json.dump(rules,f,indent=4,ensure_ascii=False)
    

def header_extractor(texte_Rule):
    lignes=texte_Rule.split("Profile Applicability:")
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
    parenthese_droite=header.find(")",parenthese_gauche)
    assessment=header[parenthese_gauche+1:parenthese_droite]
    if assessment:
        return assessment
    return None

def profile_extractor(texte_Rule):
    start=texte_Rule.find("Profile Applicability:")+len("Profile Applicability:")+1
    end=texte_Rule.find("Description:",start)-1
    return texte_Rule[start:end]


def description_extractor(texte_Rule):
    start=texte_Rule.find("Description:")+len("Description:")+1
    end=texte_Rule.find("Rationale:",start)-1
    return texte_Rule[start:end]


def rationale_extractor(texte_Rule):
    start=texte_Rule.find("Rationale:")+len("Rationale:")+1
    if(texte_Rule.find("Impact:",start)!=-1):
        end=texte_Rule.find("Impact:",start)-1
    else:
        end=texte_Rule.find("Audit:",start)-1
    return texte_Rule[start:end]


def impact_extractor(texte_Rule):
    start=texte_Rule.find("Impact:")
    if(start==-1):
        return ""
    else:
        start+=len("Impact:")+1
        end=texte_Rule.find("Audit:",start)-1
        return texte_Rule[start:end]


def audit_extractor(texte_Rule):
    start=texte_Rule.find("Audit:")+len("Audit:")+1
    end=texte_Rule.find("Remediation:",start)-1
    return texte_Rule[start:end]


def remediation_extractor(texte_Rule):
    start=texte_Rule.find("Remediation:")+len("Remediation:")+1
    end=0
    if(texte_Rule.find("Default Value:",start)!=-1):
        end=texte_Rule.find("Default Value:",start)-1
    else:
        end=texte_Rule.find("References:",start)-1
    return texte_Rule[start:end]


def default_value_extractor(texte_Rule):
    start=texte_Rule.find("Default Value:")
    if start==-1:
        return ""
    else:
        start+=len("Default Value:")+1
        end=texte_Rule.find("References:",start)-1
        return texte_Rule[start:end]
    

def references_extractor(texte_Rule):
    start=texte_Rule.find("References:")+len("References:")+1
    end=texte_Rule.find("Additional Information:",start)
    if end==-1:
        end=texte_Rule.find("CIS Controls:",start)
    end=end-1
    return texte_Rule[start:end]


def additional_info_extractor(texte_Rule):
    start=texte_Rule.find("Additional Information:")
    if start==-1:
        return ""
    start+=len("Additional Information:")+1
    end=texte_Rule.find("CIS Controls:",start)-1
    return texte_Rule[start:end]


def cis_controls_extractor(texte_Rule):
    start=texte_Rule.find("CIS Controls:")+len("CIS Controls:")+1
    return texte_Rule[start:]


RuleExtractor(liste_num_pages)