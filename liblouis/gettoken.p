static pass_Codes
emphGetToken ()
{
  while (passLinepos < passLine.length)
    {
      passPrevLinepos = passLinepos;
      switch (passLine.chars[passLinepos])
	{
	case 'a':
	  if (passHaveKeyword ("ttr"))
	    return pass_attributes;
	  passGetName ();
	  return pass_nameFound;
	case 'b':
	  if (passHaveKeyword ("ack"))
	    return pass_lookback;
	  if (passHaveKeyword ("ool"))
	    return pass_boolean;
	  passGetName ();
	  return pass_nameFound;
	case 'c':
	  if (passHaveKeyword ("lass"))
	    return pass_class;
	  passGetName ();
	  return pass_nameFound;
	case 'd':
	  if (passHaveKeyword ("ef"))
	    return pass_define;
	  passGetName ();
	  return pass_nameFound;
	case 'e':
	  if (passHaveKeyword ("mph"))
	    return pass_emphasis;
	  passGetName ();
	  return pass_nameFound;
	case 'f':
	  if (passHaveKeyword ("ind"))
	    return pass_search;
	  if (passHaveKeyword ("irst"))
	    return pass_first;
	  passGetName ();
	  return pass_nameFound;
	case 'g':
	  if (passHaveKeyword ("roup"))
	    return pass_group;
	  passGetName ();
	  return pass_nameFound;
	case 'i':
	  if (passHaveKeyword ("f"))
	    return pass_if;
	  passGetName ();
	  return pass_nameFound;
	case 'l':
	  if (passHaveKeyword ("ast"))
	    return pass_last;
	  passGetName ();
	  return pass_nameFound;
	case 'm':
	  if (passHaveKeyword ("ark"))
	    return pass_mark;
	  passGetName ();
	  return pass_nameFound;
	case 'r':
	  if (passHaveKeyword ("epgroup"))
	    return pass_repGroup;
	  if (passHaveKeyword ("epcopy"))
	    return pass_copy;
	  if (passHaveKeyword ("epomit"))
	    return pass_omit;
	  if (passHaveKeyword ("ep"))
	    return pass_replace;
	  passGetName ();
	  return pass_nameFound;
	case 's':
	  if (passHaveKeyword ("cript"))
	    return pass_script;
	  if (passHaveKeyword ("wap"))
	    return pass_swap;
	  passGetName ();
	  return pass_nameFound;
	case 't':
	  if (passHaveKeyword ("hen"))
	    return pass_then;
	  passGetName ();
	  return pass_nameFound;
	default:
	  if (passLine.chars[passLinepos] <= 32)
	    {
	      passLinepos++;
	      break;
	    }
	  if (passLine.chars[passLinepos] >= '0'
	      && passLine.chars[passLinepos] <= '9')
	    {
	      passGetNumber ();
	      return pass_numberFound;
	    }
	  else
	    {
	      if (!passGetName ())
		return pass_invalidToken;
	      else
		return pass_nameFound;
	    }
	}
    }
  return pass_noMoreTokens;
}
