import yaml

# add additional constructor
def secret_constructor(loader: yaml.SafeLoader, node: yaml.nodes.ScalarNode): 
    return "!secret " + f"{loader.construct_scalar(node)}"

yaml.SafeLoader.add_constructor('!secret', secret_constructor)

# load yaml file
with open('examples/full_de.yaml', 'r') as file:
    config = yaml.safe_load(file)

# get block external components and load it into a new dict
external_components_block = {'external_components': config.get('external_components')}

print(yaml.safe_dump(external_components_block))
