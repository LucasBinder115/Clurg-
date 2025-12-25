-- Schema para o banco de dados Clurg
-- Versão: 1.0
-- Data: 2025-01-21

-- Criar schema se não existir
CREATE SCHEMA IF NOT EXISTS clurg;

-- Tabela de projetos
-- Armazena informações básicas dos projetos versionados
CREATE TABLE IF NOT EXISTS clurg.projects (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL UNIQUE,
    description TEXT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Tabela de commits
-- Indexa os commits do filesystem (source of truth)
CREATE TABLE IF NOT EXISTS clurg.commits (
    id SERIAL PRIMARY KEY,
    project_id INTEGER REFERENCES clurg.projects(id),
    commit_hash VARCHAR(64) NOT NULL UNIQUE,
    message TEXT,
    author VARCHAR(255),
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    parent_hash VARCHAR(64),
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Índices para performance
CREATE INDEX IF NOT EXISTS idx_commits_project_id ON clurg.commits(project_id);
CREATE INDEX IF NOT EXISTS idx_commits_hash ON clurg.commits(commit_hash);
CREATE INDEX IF NOT EXISTS idx_commits_timestamp ON clurg.commits(timestamp);

-- Índice para projetos
CREATE INDEX IF NOT EXISTS idx_projects_name ON clurg.projects(name);