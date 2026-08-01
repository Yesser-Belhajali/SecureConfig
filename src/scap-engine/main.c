#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>

struct rule_node{
    struct xccdf_rule *rule;
    struct rule_node *next;
};

struct rule_node *push_front(struct rule_node *head, struct xccdf_rule *rule,bool *error){
    struct rule_node *rule_node=malloc(sizeof(struct rule_node));
    if(rule_node==NULL){
        *error=true;
        return head;
    }
    rule_node->rule=rule;
    rule_node->next=head;
    return rule_node;
}

void free_rule_list(struct rule_node *head){
    while(head!=NULL){
        struct rule_node *next=head->next;
        free(head);
        head=next;
    }
}


struct rule_node *collect_rules_recursive(struct xccdf_item *item, struct rule_node *head,bool *error){
    if(*error){
        return head;
    }
    xccdf_type_t item_type=xccdf_item_get_type(item);
    if(item_type==XCCDF_RULE){
        head=push_front(head,(struct xccdf_rule *)item,error);
    }
    else if(item_type==XCCDF_GROUP){
        struct xccdf_item_iterator *child_it=xccdf_group_get_content((struct xccdf_group *)item);
        if(child_it==NULL){
            *error=true;
            return head;
        }
        while(xccdf_item_iterator_has_more(child_it) && !*error){
            struct xccdf_item *child=xccdf_item_iterator_next(child_it);
            head=collect_rules_recursive(child,head,error);
        }
        xccdf_item_iterator_free(child_it);
    }
    return head;
}


struct rule_node *get_all_rules(struct xccdf_benchmark *benchmark,bool *error) {
    struct rule_node *head = NULL;
    struct xccdf_item_iterator *root_it = xccdf_benchmark_get_content(benchmark);
    if(root_it==NULL){
        *error=true;
        return NULL;
    }
    while (xccdf_item_iterator_has_more(root_it) && !*error) {
        struct xccdf_item *item = xccdf_item_iterator_next(root_it);
        head = collect_rules_recursive(item, head,error);
    }
    xccdf_item_iterator_free(root_it);
    return head;
}


const char *get_first_title(struct xccdf_rule *rule) {
    struct oscap_text_iterator *title_it = xccdf_rule_get_title(rule);
    const char *title = NULL;
    if (title_it!=NULL && oscap_text_iterator_has_more(title_it)) {
        struct oscap_text *text = oscap_text_iterator_next(title_it);
        title = oscap_text_get_text(text);
    }
    if(title_it!=NULL){
        oscap_text_iterator_free(title_it);
    }
    return title;
}


int main(int argc, char** argv){
    if(argc!=2){
        printf("Vous avez fourni %d paramètres alors qu'on a besoin de seulement 1",argc-1);
        return 1;
    }
    oscap_init();

    struct xccdf_session* session=xccdf_session_new(argv[1]);

    if(session==NULL){
        printf("Echec dans l'initialisation de la session\n");
        oscap_cleanup();
        return 1;
    }

    if(xccdf_session_load(session)!=0){
        printf("Le chargement des composants a échoué!!!\n");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_policy_model *policy_model=xccdf_session_get_policy_model(session);
    if(policy_model==NULL){
        printf("Echech dans la récupération de la policy_model!!!!\n");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_benchmark *benchmark=xccdf_policy_model_get_benchmark(policy_model);
    if(benchmark==NULL){
        printf("Echec dans la récupération du benchmark!!!!\n");
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }

    bool error=false;

    struct rule_node *all_rules=get_all_rules(benchmark,&error);

    if(error){
        fprintf(stderr, "Erreur: échec lors de la collecte des règles (allocation mémoire ou itérateur invalide)\n");
        free_rule_list(all_rules); // libère ce qui a pu être construit avant l'échec
        xccdf_session_free(session);
        oscap_cleanup();
        return 1;
    }


    for (struct rule_node *cur = all_rules; cur != NULL; cur = cur->next) {
        const char *id = xccdf_rule_get_id(cur->rule);
        if(id == NULL){
            fprintf(stderr, "Erreur: une règle sans ID a été rencontrée\n");
            free_rule_list(all_rules);
            xccdf_session_free(session);
            oscap_cleanup();
            return 1;
        }
        const char *title = get_first_title(cur->rule);
        printf("ID: %s\nTitre: %s\n\n", id, title ? title : "N/A");
    }


    
    free_rule_list(all_rules);
    xccdf_session_free(session);
    oscap_cleanup();
    return 0;
}
