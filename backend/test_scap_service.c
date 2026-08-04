#include <stdio.h>
#include "scap_service.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fichier-ds.xml>\n", argv[0]);
        return 1;
    }

    struct profile_list *profiles = NULL;
    int count = list_profiles_for_ds(argv[1], &profiles);

    if (count < 0) {
        fprintf(stderr, "Echec du chargement des profils\n");
        return 1;
    }

    printf("%d profils trouvés :\n", count);
    for (int i = 0; i < count; i++) {
        printf("%d) ID : %s\n   Titre : %s\n", i + 1,
               profiles[i].id ? profiles[i].id : "(NULL)",
               profiles[i].title ? profiles[i].title : "(NULL)");
    }

    free_profile_list(profiles, count);
    return 0;
}