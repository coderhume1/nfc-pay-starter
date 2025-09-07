
#!/bin/sh
set -e

DB="$DATABASE_URL"
if [ -z "$DB" ]; then
  echo "DATABASE_URL missing, skipping Prisma"
else
  case "$DB" in
    *USER:PASSWORD*|*ep-your-project-pooler*) 
      echo "Placeholder DATABASE_URL detected, skipping Prisma"
      ;;
    *)
      npx prisma migrate deploy || npx prisma db push
      ;;
  esac
fi

next build
