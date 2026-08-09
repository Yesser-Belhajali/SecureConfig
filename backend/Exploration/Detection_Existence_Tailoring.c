#include <unistd.h>   // access()
#include <stdio.h>    // snprintf()

// Vérifie si un fichier de tailoring existe pour la distro donnée.
// id : identifiant de la distro (ex: "ubuntu2404", "rhel9")
// Retourne 1 si le fichier existe, 0 sinon.
static int has_tailoring(const char *id) {
    char tailoring_path[256];
    int n = snprintf(tailoring_path, sizeof(tailoring_path),
                      "../data/%s/ssg-%s-tailoring.xml", id, id);

    if (n < 0 || (size_t)n >= sizeof(tailoring_path)) {
        return 0; // troncature, id trop long -> pas de tailoring détecté
    }

    return access(tailoring_path, F_OK) == 0;
}


int main(void) {
    printf("ubuntu2404 -> %s\n", has_tailoring("ubuntu2404") ? "true" : "false");
    printf("debian13   -> %s\n", has_tailoring("debian13") ? "true" : "false");
    return 0;
}