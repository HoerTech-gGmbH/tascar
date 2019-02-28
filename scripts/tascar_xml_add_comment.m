function tascar_xml_add_comment(doc,elem,sComment)
    comment = javaMethod('createComment',doc,sComment);
    javaMethod('insertBefore',javaMethod('getParentNode',elem),comment, ...
               elem );
    