#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xccdf_session.h>
#include <xccdf_policy.h>
#include <xccdf_benchmark.h>
#include <ds_sds_session.h>
#include <oscap_source.h>


struct rule_node{
    struct xccdf_rule *rule;
    struct rule_node *next;
};


void free_rule_list(struct rule_node *head){
    while(head!=NULL){
        struct rule_node *new_rule_node=head;
        head=head->next;
        free(new_rule_node);
    }
}

struct rule_node *push_front(struct rule_node *head,struct xccdf_rule *rule,bool *error){
    struct rule_node *new_rule_node=malloc(sizeof(struct rule_node));
    if(new_rule_node==NULL){
        printf("Echec dans la création d'un noeud rule_node!!!!!\n");
        *error=true;
        return head;
    }
    new_rule_node->rule=rule;
    new_rule_node->next=head;
    return new_rule_node;
}

struct rule_node *collect_rules_recursive(struct xccdf_item *benchmark_item,struct rule_node *head,bool *error){
    if(*error){
        return head;
    }
    xccdf_type_t benchmark_item_type=xccdf_item_get_type(benchmark_item);
    if(benchmark_item_type==XCCDF_RULE){
        head=push_front(head,xccdf_item_to_rule(benchmark_item),error);
    }
    else if(benchmark_item_type==XCCDF_GROUP){
        struct xccdf_item_iterator *group_iterator=xccdf_group_get_content(xccdf_item_to_group(benchmark_item));
        if(group_iterator==NULL){
            printf("Erreur dans la création de l'itérateur du groupe!!!!\n");
            *error=true;
            return head;
        }
        while(xccdf_item_iterator_has_more(group_iterator) && *error==false){
            struct xccdf_item *group_item=xccdf_item_iterator_next(group_iterator);
            head=collect_rules_recursive(group_item,head,error);
        }
        xccdf_item_iterator_free(group_iterator);
    }
    return head;
}

struct rule_node *get_benchmark_rules(struct xccdf_benchmark *benchmark,bool *error){
    struct rule_node *head=NULL;
    struct xccdf_item_iterator *benchmark_iterator=xccdf_benchmark_get_content(benchmark);
    if(benchmark_iterator==NULL){
        printf("Erreur dans la création de l'itérateur du benchmark!!!!\n");
        *error=true;
        return NULL;
    }
    while(xccdf_item_iterator_has_more(benchmark_iterator) && *error==false){
        struct xccdf_item *benchmark_item=xccdf_item_iterator_next(benchmark_iterator);
        head=collect_rules_recursive(benchmark_item,head,error);
    }
    xccdf_item_iterator_free(benchmark_iterator);
    return head;
}

struct rule_node *get_benchmark_rules_or_null(struct xccdf_benchmark *benchmark){
    bool error=false;
    struct rule_node *head=get_benchmark_rules(benchmark,&error);
    if(error){
        free_rule_list(head);
        return NULL;
    }
    return head;
}


const char *get_rule_title(struct xccdf_rule *rule) {
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


int main(int argc,char **argv){
    if(argc!=2){
        printf("Vous avez fourni %d paramètres alors qu'on a besoin de 2\n",argc-1);
        return 1;
    }

    oscap_init();

    struct oscap_source *oscap_ds_source=oscap_source_new_from_file(argv[1]);
    if(oscap_ds_source==NULL){
        printf("Erreur dans l'initialisationde la source!!!!\n");
        oscap_cleanup();
        return 1;
    }

    struct ds_sds_session *ds_sds_session=ds_sds_session_new_from_source(oscap_ds_source);
    if(ds_sds_session==NULL){
        printf("Erreur dans la conversion de la source vers une Data Stream Source!!!!\n");
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }
    struct oscap_source *oscap_xccdf_source=ds_sds_session_select_checklist(ds_sds_session,NULL,NULL,NULL);
    if(oscap_xccdf_source==NULL){
        printf("Erreur dans la création de la source XCCDF depuis la Data Stream source!!!\n");
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    struct xccdf_benchmark *benchmark=xccdf_benchmark_import_source(oscap_xccdf_source);
    if(benchmark==NULL){
        printf("Erreur dans la création du benchmark!!!!\n");
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    bool error=false;

    struct rule_node *head=get_benchmark_rules_or_null(benchmark,&error);

    if(error){
        printf("Erreur: échec lors de la collecte des règles (allocation mémoire ou itérateur invalide)\n");
        free_rule_list(head);
        xccdf_benchmark_free(benchmark);
        ds_sds_session_free(ds_sds_session);
        oscap_source_free(oscap_ds_source);
        oscap_cleanup();
        return 1;
    }

    struct rule_node *iterator=head;

    while(iterator!=NULL){
        printf("Titre : %s\nID : %s\n",get_rule_title(iterator->rule),xccdf_rule_get_id(iterator->rule));
        iterator=iterator->next;
    }

    free_rule_list(head);
    xccdf_benchmark_free(benchmark);
    ds_sds_session_free(ds_sds_session);
    oscap_source_free(oscap_ds_source);
    oscap_cleanup();
    return 0;
}